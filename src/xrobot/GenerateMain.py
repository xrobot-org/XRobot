"""Generate an ordered, static C++ application from source interfaces and YAML."""
import argparse
import os
import re
import tempfile
from pathlib import Path
import yaml
from xrobot.CppSource import tokens, code_tokens, close_token, split_arguments, bind_identifiers
from xrobot.ModuleParser import discover_modules, select_module, source_interface


class ConfigLoader(yaml.BaseLoader):
    """Preserve C++ scalar spelling (including on/off, hex and date-like tokens)."""


def _mapping(loader, node):
    result = {}
    for key_node, value_node in node.value:
        key = loader.construct_object(key_node)
        if not isinstance(key, str) or key in result:
            raise ValueError('Duplicate or non-scalar YAML key at line %d' % (key_node.start_mark.line + 1))
        result[key] = loader.construct_object(value_node)
    return result


ConfigLoader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, _mapping)


def load_config(path: Path) -> dict:
    result = yaml.load(Path(path).read_text(encoding='utf-8-sig'), Loader=ConfigLoader)
    if not isinstance(result, dict):
        raise ValueError('%s: expected an application configuration mapping' % path)
    validate_config(result)
    return result


def cpp_text(value, field):
    if isinstance(value, bool):
        return 'true' if value else 'false'
    if isinstance(value, (int, float)):
        return str(value)
    if not isinstance(value, str) or not value.strip():
        raise ValueError('%s must contain C++ argument text, not YAML aggregates/null' % field)
    if value.lstrip().startswith('@'):
        raise ValueError('%s: use ordinary C++ text instead of @ expressions' % field)
    return value


def validate_config(config):
    extra = set(config) - {'modules', 'settings'}
    if extra:
        raise ValueError('Unsupported application fields: %s; migrate to module/id/args and settings' % ', '.join(sorted(extra)))
    entries = config.get('modules', [])
    if not isinstance(entries, list):
        raise ValueError('modules must be an ordered list')
    used = set()
    for i, entry in enumerate(entries):
        if not isinstance(entry, dict) or set(entry) - {'module', 'id', 'args', 'template_args'}:
            raise ValueError('modules[%d] requires module/id and ordered args/template_args' % i)
        for key in ('module', 'id'):
            if not isinstance(entry.get(key), str) or not entry[key]:
                raise ValueError('modules[%d].%s is required' % (i, key))
        if entry['id'] in used:
            raise ValueError('Duplicate instance id: ' + entry['id'])
        if entry['id'].startswith('xr_') or entry['id'] == 'XRobotMonitorAll':
            raise ValueError('Instance id collides with generated identifiers: ' + entry['id'])
        used.add(entry['id'])
        for key in ('args', 'template_args'):
            values = entry.get(key, [])
            if not isinstance(values, list):
                raise ValueError('modules[%d].%s must be an ordered list' % (i, key))
            for j, value in enumerate(values):
                cpp_text(value, 'modules[%d].%s[%d]' % (i, key, j))
    settings = config.get('settings', {})
    if not isinstance(settings, dict) or set(settings) - {'monitor_sleep_ms'}:
        raise ValueError('settings only accepts monitor_sleep_ms')
    sleep = str(settings.get('monitor_sleep_ms', 1000))
    if not re.fullmatch(r'\d+', sleep) or int(sleep) > 0xffffffff:
        raise ValueError('monitor_sleep_ms must be an unsigned 32-bit millisecond count')


def atomic_write(path: Path, text: str):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_bytes() == text.encode('utf-8'):
        return
    handle, temporary = tempfile.mkstemp(prefix=path.name + '.', suffix='.tmp', dir=str(path.parent))
    try:
        with os.fdopen(handle, 'w', encoding='utf-8', newline='\n') as stream:
            stream.write(text)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def read_registrations(paths):
    records, names = [], set()
    for path in paths:
        path = Path(path)
        text = path.read_text(encoding='utf-8-sig')
        items = code_tokens(text)
        for i, token in enumerate(items):
            if token.text != 'XR_REGISTER' or i+1 >= len(items) or items[i+1].text != '(':
                continue
            end = close_token(items, i+1)
            parts = split_arguments(text[items[i+1].end:items[end].start])
            if len(parts) < 2 or not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', parts[0]):
                raise ValueError('%s: XR_REGISTER requires an existing name and explicit object types' % path)
            name = parts[0]
            if name in names:
                raise ValueError('Duplicate XR_REGISTER name: ' + name)
            if name.startswith('xr_'):
                raise ValueError('Registration collides with generated identifier prefix: ' + name)
            if len(set(parts[1:])) != len(parts[1:]):
                raise ValueError('Duplicate view in XR_REGISTER: ' + name)
            # Typedefs are deliberately left to C++; this rejects literal T& spellings only.
            if any(p.rstrip().endswith('&') for p in parts[1:]):
                raise ValueError('Register object types, not reference types: ' + name)
            names.add(name)
            records.append({'name': name, 'types': parts[1:], 'source': str(path), 'line': text.count('\n', 0, token.start)+1})
    return records


HELPERS = r'''namespace xrobot_generated {
template <typename...> struct TypeList {};
template <typename Source, typename... Views>
struct RegistrationMatches
    : std::bool_constant<(!std::is_reference<Views>::value && ...) &&
                         (std::is_convertible<Source*, Views*>::value && ...)> {};

template <typename View, typename Source>
inline void* Erase(Source& source) noexcept {
  static_assert(!std::is_reference<View>::value, "Register an object type, not T&");
  // Implicit conversion adjusts base subobjects and rejects pointer covariance.
  View* view = std::addressof(source);
  return const_cast<void*>(static_cast<const volatile void*>(view));
}

template <typename> struct MonitorSignature : std::false_type {};
template <typename T> struct MonitorSignature<void (T::*)()> : std::true_type {};
template <typename T> struct MonitorSignature<void (T::*)() noexcept> : std::true_type {};
template <typename T> struct MonitorSignature<void (T::*)() const> : std::true_type {};
template <typename T> struct MonitorSignature<void (T::*)() const noexcept> : std::true_type {};
template <typename T> struct MonitorSignature<void (T::*)() &> : std::true_type {};
template <typename T> struct MonitorSignature<void (T::*)() & noexcept> : std::true_type {};
template <typename T, typename = void> struct HasMonitor : std::false_type {};
template <typename T>
struct HasMonitor<T, std::void_t<decltype(&T::OnMonitor)>>
    : MonitorSignature<decltype(&T::OnMonitor)> {};

template <typename T> inline void Monitor(T& instance) {
  if constexpr (HasMonitor<T>::value) {
    instance.OnMonitor();
  }
}
'''


def generate_xrobot_main_code(config, modules, registrations=None):
    validate_config(config)
    registrations = registrations or []
    views, bindings = [], {}
    for record in registrations:
        fields = []
        for i, cpp_type in enumerate(record['types']):
            field = 'xr_view_%s_%d' % (record['name'], i)
            fields.append(field)
            views.append((record['name'], cpp_type, field))
        bindings[record['name']] = fields
    selected, entries = {}, []
    for i, entry in enumerate(config.get('modules', [])):
        if entry['id'] in bindings:
            raise ValueError('Instance id collides with a registered external name: ' + entry['id'])
        module = select_module(modules, entry['module'])
        if not module['manifest'].standalone:
            raise ValueError('%s is a non-standalone library, not an instance' % module['id'])
        if module['name'] in selected and selected[module['name']]['id'] != module['id']:
            raise ValueError('Two selected packages define global class %s; choose one implementation' % module['name'])
        selected[module['name']] = module
        interface = source_interface(module['header'])
        args = [bind_identifiers(cpp_text(v, 'args'), bindings) for v in entry.get('args', [])]
        template_args = [bind_identifiers(cpp_text(v, 'template_args'), bindings) for v in entry.get('template_args', [])]
        cpp_type = module['name']
        if template_args or interface['template'] is not None:
            cpp_type += '<' + ', '.join(template_args) + '>'
        entries.append((entry['id'], cpp_type, args, i))
    lines = ['#pragma once', '', '#include <array>', '#include <memory>', '#include <type_traits>', '#include "thread.hpp"']
    lines += ['#include "%s.hpp"' % name for name in selected]
    lines += ['', HELPERS]
    if views:
        lines.append('template <%s>' % ', '.join('typename XrView%d' % i for i in range(len(views))))
    lines.append('[[noreturn]] inline void Main(void* const* xr_slots) {')
    if not views:
        lines.append('  (void)xr_slots;')
    for i, (_, _, field) in enumerate(views):
        lines.append('  [[maybe_unused]] auto& %s = *static_cast<XrView%d*>(xr_slots[%d]);' % (field, i, i))
    for identity, cpp_type, args, i in entries:
        lines.append('  // modules[%d]: %s' % (i, identity))
        if args:
            lines.append('  %s %s(\n      %s\n  );' % (cpp_type, identity, '\n      , '.join(args)))
        else:
            lines.append('  %s %s;' % (cpp_type, identity))
    lines.append('  const auto XRobotMonitorAll = [&]() {')
    lines += ['    Monitor(%s);' % entry[0] for entry in entries]
    lines += ['  };', '  for (;;) {', '    XRobotMonitorAll();', '    LibXR::Thread::Sleep(%s);' % config.get('settings', {}).get('monitor_sleep_ms', 1000), '  }', '}', '}  // namespace xrobot_generated', '']
    # Macro arguments are checked at the source declaration, with no runtime table.
    for record in registrations:
        lines.append('#define XR_REGISTER_DETAIL_%s(...) \\' % record['name'])
        lines.append('  static_assert(std::is_same<xrobot_generated::TypeList<__VA_ARGS__>, \\')
        lines.append('      xrobot_generated::TypeList<%s>>::value && \\' % ', '.join(record['types']))
        lines.append('      xrobot_generated::RegistrationMatches< \\')
        lines.append('          std::remove_reference_t<decltype(%s)>, __VA_ARGS__>::value, \\' % record['name'])
        lines.append('      "XR_REGISTER changed; regenerate xrobot_main.hpp")')
    lines += ['#define XR_REGISTER(name, ...) XR_REGISTER_DETAIL_##name(__VA_ARGS__)', '', '#define XROBOT_MAIN() \\', '  do { \\']
    if views:
        lines.append('    std::array<void*, %d> xr_slots{{ \\' % len(views))
        for name, cpp_type, _ in views:
            lines.append('        xrobot_generated::Erase<%s>(%s), \\' % (cpp_type, name))
        lines.append('    }}; \\')
        lines.append('    xrobot_generated::Main<%s>(xr_slots.data()); \\' % ', '.join(v[1] for v in views))
    else:
        lines.append('    xrobot_generated::Main(nullptr); \\')
    lines += ['  } while (false)', '']
    return '\n'.join(lines)


def generate(config_path=Path('User/xrobot.yaml'), modules_dir=Path('Modules'), output=Path('User/xrobot_main.hpp'), register_sources=None, lock_path=None):
    config = load_config(Path(config_path))
    sources = list(register_sources or [])
    if not sources:
        default = Path(config_path).parent / 'app_main.cpp'
        if default.exists():
            sources = [default]
    if any(Path(p).resolve() == Path(output).resolve() for p in sources):
        raise ValueError('Registration input must be original source, not the generated header')
    registrations = read_registrations(sources)
    modules = discover_modules(Path(modules_dir), lock_path)
    code = generate_xrobot_main_code(config, modules, registrations)
    atomic_write(Path(output), code)
    for record in registrations:
        fields = ['xr_view_%s_%d' % (record['name'], i) for i in range(len(record['types']))]
        print('%s: %s' % (record['name'], ', '.join('%s (%s)' % pair for pair in zip(fields, record['types']))))
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-c', '--config', default='User/xrobot.yaml')
    parser.add_argument('-d', '--directory', default='Modules')
    parser.add_argument('-o', '--output', default='User/xrobot_main.hpp')
    parser.add_argument('--register-source', action='append', default=[])
    parser.add_argument('--lock')
    args = parser.parse_args()
    try:
        generate(args.config, args.directory, args.output, args.register_source, args.lock)
        print('Generated ' + args.output)
    except (OSError, ValueError, TypeError, yaml.YAMLError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
