"""Generate an ordered, static C++ application from source interfaces and YAML."""
import argparse
import os
import re
import tempfile
from pathlib import Path
import yaml
from xr_syntax.cpp import CppDocument, identifier_occurrences

from xrobot.SourceSyntax import code_tokens, close_token, split_arguments, bind_identifiers
from xrobot.ModuleParser import discover_modules, select_module, source_interface
from xrobot.ConstructorModel import (scalar_text, initial_arguments, template_bindings,
                                     construct_arguments)


class ConfigLoader(yaml.BaseLoader):
    """Preserve C++ spelling while distinguishing unfilled YAML null values."""

    def construct_scalar(self, node):
        if node.style is None and node.value in ('', '~', 'null', 'Null', 'NULL'):
            return None
        return super().construct_scalar(node)


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
    return scalar_text(value, field)


def validate_value(value, path):
    if value is None:
        return  # A saved, unfilled configuration is valid; generation is not.
    if isinstance(value, dict):
        for name, child in value.items():
            if not isinstance(name, str) or not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', name):
                raise ValueError(path + ': invalid aggregate field name')
            validate_value(child, path+'.'+name)
    elif isinstance(value, list):
        for i, child in enumerate(value):
            validate_value(child, path+'[%d]' % i)
    else:
        cpp_text(value, path)


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
        if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', entry['id']):
            raise ValueError('Invalid C++ instance identifier: ' + entry['id'])
        values = entry.get('args', [])
        if not isinstance(values, list):
            raise ValueError('modules[%d].args must be an ordered list' % i)
        names = set()
        for j, value in enumerate(values):
            if not isinstance(value, dict) or len(value) != 1:
                raise ValueError('modules[%d].args[%d] requires one named parameter' % (i, j))
            name, argument = next(iter(value.items()))
            if not isinstance(name, str) or not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', name) or name in names:
                raise ValueError('Invalid or duplicate constructor parameter: ' + str(name))
            names.add(name)
            validate_value(argument, entry['id']+'.args.'+name)
        templates = entry.get('template_args', [])
        if not isinstance(templates, list):
            raise ValueError('modules[%d].template_args must be an ordered list' % i)
        for j, value in enumerate(templates):
            if value is not None:
                cpp_text(value, 'modules[%d].template_args[%d]' % (i, j))
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


def caller_defined_names(items, stop):
    """Conservatively defer names declared by the BSP to the call site.

    The generated header can be included before these declarations, even at
    namespace scope. This is name collection, not C++ scope/type resolution;
    an extra view template parameter is safer than an invisible type name.
    """
    names = set()
    depth = 0
    for i, token in enumerate(items[:stop]):
        text = token.text
        if text == '{':
            if depth and i and items[i-1].kind == 'identifier':
                names.add(items[i-1].text)  # Includes constexpr N{2}.
            depth += 1
        elif text == '}':
            depth = max(0, depth-1)
        elif text == '=' and i and items[i-1].kind == 'identifier':
            names.add(items[i-1].text)  # Includes enumerators and array extents.
        elif text in ('class', 'struct', 'enum', 'typename') and i+1 < stop:
            j = i+1
            if items[j].text in ('class', 'struct') and j+1 < stop:
                j += 1
            if items[j].kind == 'identifier':
                names.add(items[j].text)
            if text == 'enum':
                while j < stop and items[j].text not in ('{', ';'):
                    j += 1
                if j < stop and items[j].text == '{':
                    end = close_token(items, j)
                    for k in range(j+1, min(end, stop)):
                        if items[k].kind == 'identifier' and items[k-1].text in ('{', ','):
                            names.add(items[k].text)
        elif text == 'template' and i+1 < stop and items[i+1].text == '<':
            end = close_token(items, i+1)
            parameters = ' '.join(t.text for t in items[i+2:end])
            for parameter in split_arguments(parameters):
                ts = code_tokens(parameter)
                equal = next((j for j, t in enumerate(ts) if t.text == '='), len(ts))
                if equal and ts[equal-1].kind == 'identifier':
                    names.add(ts[equal-1].text)
        elif text == 'using' and i+1 < stop:
            if items[i+1].text == 'namespace':
                if depth:
                    names.add('*')
            else:
                j = i+1
                while j+2 < stop and items[j+1].text == '::':
                    j += 2
                if items[j].kind == 'identifier':
                    names.add(items[j].text)
        elif text == 'typedef':
            j = i+1
            while j < stop and items[j].text != ';':
                # A member declaration inside an anonymous struct is not the
                # end of its typedef. Skip balanced declarators as one unit.
                if items[j].text in ('{', '(', '[', '<'):
                    j = close_token(items, j)+1
                else:
                    j += 1
            # Also covers arrays and function-pointer typedef declarators.
            names.update(t.text for t in items[i+1:j] if t.kind == 'identifier'
                         and t.text not in ('void', 'bool', 'char', 'short', 'int',
                                            'long', 'float', 'double', 'signed',
                                            'unsigned', 'const', 'volatile'))
    return names


def caller_scoped_view(cpp_type, names):
    identifiers = {t.text for t in code_tokens(cpp_type) if t.kind == 'identifier'}
    return bool(identifiers & names) or 'decltype' in identifiers or '*' in names


def read_registrations(paths):
    """Read XR_REGISTER invocations through xr-syntax while preserving XRobot contracts."""
    records, names = [], set()
    for path in paths:
        path = Path(path)
        text = path.read_text(encoding='utf-8-sig', errors='surrogateescape')
        document = CppDocument.parse(text, source_name=str(path))
        invocations = document.invocation_views('XR_REGISTER', template_angles=True)
        candidates = [
            occurrence
            for occurrence in identifier_occurrences(text)
            if occurrence.text == 'XR_REGISTER' and occurrence.following == '('
        ]
        if len(invocations) != len(candidates):
            raise ValueError('%s: malformed XR_REGISTER invocation' % path)

        encoded = document.render_bytes()
        for invocation in invocations:
            parts = list(invocation.arguments)
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
            prefix = encoded[:invocation.span.start].decode(
                'utf-8', errors='surrogateescape'
            )
            prefix_tokens = code_tokens(prefix)
            local_names = caller_defined_names(prefix_tokens, len(prefix_tokens))
            records.append({
                'name': name,
                'types': parts[1:],
                'source': str(path),
                'line': invocation.line,
                'caller_views': [
                    typ for typ in parts[1:]
                    if caller_scoped_view(typ, local_names)
                ],
            })
    return records


HELPERS = r'''namespace xrobot_generated {
template <typename...> struct TypeList {};
template <typename Source, typename... Views>
struct RegistrationMatches
    : std::bool_constant<(!std::is_reference<Views>::value && ...) &&
                         (std::is_convertible<Source*, Views*>::value && ...)> {};

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
}  // namespace xrobot_generated
'''


def generate_xrobot_main_code(config, modules, registrations=None, compile_check=False):
    validate_config(config)
    registrations = registrations or []
    views, bindings, typed_views = [], {}, {}
    for record in registrations:
        fields = []
        for i, cpp_type in enumerate(record['types']):
            field = 'xr_view_%s_%d' % (record['name'], i)
            fields.append(field)
            views.append((record['name'], cpp_type, field))
        bindings[record['name']] = fields
        typed_views[record['name']] = list(zip(record['types'], fields))
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
        template_args = [bind_identifiers(cpp_text(v, 'template_args'), bindings) for v in entry.get('template_args', [])]
        cpp_type = module['name']
        if template_args or interface['template'] is not None:
            cpp_type += '<' + ', '.join(template_args) + '>'
        templates = template_bindings(interface, template_args)
        declarations, args = construct_arguments(interface, entry.get('args', []),
            cpp_type, templates, typed_views, bindings, entry['id'], compile_check)
        entries.append((entry['id'], cpp_type, args, i, declarations))
        typed_views[entry['id']] = [(cpp_type, entry['id'])]
    # Only transport consumed views, while checking every XR_REGISTER declaration.
    used_fields = set()
    for _, cpp_type, args, _, declarations in entries:
        for expression in [cpp_type] + args + declarations:
            used_fields.update(token.text for token in code_tokens(expression)
                               if token.kind == 'identifier')
    views = [view for view in views if view[2] in used_fields]
    counts = {name: sum(v[0] == name for v in views) for name, _, _ in views}
    type_names = {t.text for _, typ, _ in views for t in code_tokens(typ)
                  if t.kind == 'identifier'} | set(selected)
    renamed = {}
    for name, _, field in views:
        # Preserve readable BSP names unless multiple consumed views or C++ type
        # shadowing require the existing unambiguous internal name.
        renamed[field] = [name if counts[name] == 1 and name not in type_names
                          and name not in ('std', 'LibXR', 'xrobot_generated') else field]
    entries = [(identity, bind_identifiers(typ, renamed),
                [bind_identifiers(arg, renamed) for arg in args], i,
                [bind_identifiers(d, renamed) for d in declarations])
               for identity, typ, args, i, declarations in entries]
    caller_views = {(r['name'], typ) for r in registrations
                    for typ in r.get('caller_views', [])}
    local_templates, parameters, actual_types = [], [], []
    template_names = type_names | {entry[0] for entry in entries} | {v[0] for v in views}
    for i, (name, typ, field) in enumerate(views):
        if (name, typ) in caller_views:
            parameter_type = 'xr_view_type_%d' % i
            while parameter_type in template_names:
                parameter_type += '_'
            template_names.add(parameter_type)
            local_templates.append('typename ' + parameter_type)
            actual_types.append(typ)
            typ = parameter_type
        declaration = ('std::add_lvalue_reference_t<%s>' % typ
                       if any(t.text in ('(', '[') for t in code_tokens(typ))
                       else typ + '&')
        parameters.append('%s %s' % (declaration, renamed[field][0]))
    lines = ['#pragma once', '', '#include <memory>', '#include <type_traits>',
             '#include <utility>', '#include "thread.hpp"']
    lines += ['#include "%s.hpp"' % name for name in selected]
    if compile_check:
        lines = lines[2:]
    lines += ['', HELPERS]
    if compile_check:
        lines += ['namespace xrobot_generated {', 'void XRobotCompileCheck() {',
                  '  // Compilation only: this function must never be invoked.',
                  '  [[maybe_unused]] static void* xr_ci_null = static_cast<void*>(nullptr);']
    else:
        lines += ['// Force only this entry inline in optimized Clang builds.',
                  '#if defined(__clang__) && defined(__OPTIMIZE__) && !defined(LIBXR_DEBUG_BUILD) && \\',
                  '    ((defined(XROBOT_OPTIMIZED_BUILD) && XROBOT_OPTIMIZED_BUILD) || \\',
                  '     (!defined(XROBOT_OPTIMIZED_BUILD) && defined(NDEBUG)))',
                  '#define XR_XROBOT_MAIN_INLINE [[gnu::always_inline]] inline',
                  '#else', '#define XR_XROBOT_MAIN_INLINE inline', '#endif', '']
        if local_templates:
            lines.append('template <%s>' % ', '.join(local_templates))
        signature = '[[noreturn]] XR_XROBOT_MAIN_INLINE void XRobotMain('
        if parameters:
            lines += [signature, '    ' + ',\n    '.join(parameters) + ') {']
        else:
            lines.append(signature + ') {')
    for identity, cpp_type, args, i, declarations in entries:
        lines.extend(declarations)
        lines.append('  // modules[%d]: %s' % (i, identity))
        if args:
            lines.append('  static %s %s(\n      %s\n  );' % (cpp_type, identity, '\n      , '.join(args)))
        else:
            lines.append('  static %s %s;' % (cpp_type, identity))
    if compile_check:
        lines += ['  Monitor(%s);' % entry[0] for entry in entries]
        lines += ['}', '}  // namespace xrobot_generated', '']
        return '\n'.join(lines)
    lines += ['  for (;;) {']
    lines += ['    ::xrobot_generated::Monitor(%s);' % entry[0] for entry in entries]
    lines += ['    LibXR::Thread::Sleep(%s);' % config.get('settings', {}).get('monitor_sleep_ms', 1000),
              '  }', '}', '', '#undef XR_XROBOT_MAIN_INLINE', '']
    for record in registrations:
        lines.append('#define XR_REGISTER_DETAIL_%s(...) \\' % record['name'])
        lines.append('  static_assert(std::is_same<::xrobot_generated::TypeList<__VA_ARGS__>, \\')
        lines.append('      ::xrobot_generated::TypeList<%s>>::value && \\' % ', '.join(record['types']))
        lines.append('      ::xrobot_generated::RegistrationMatches< \\')
        lines.append('          std::remove_reference_t<decltype(%s)>, __VA_ARGS__>::value, \\' % record['name'])
        lines.append('      "XR_REGISTER changed; regenerate xrobot_main.hpp")')
    lines += ['#define XR_REGISTER(name, ...) XR_REGISTER_DETAIL_##name(__VA_ARGS__)', '']
    call = '::XRobotMain' + ('<%s>' % ', '.join(actual_types) if actual_types else '')
    arguments = ', '.join(name for name, _, _ in views)
    if len(call) + len(arguments) < 72:
        lines.append('#define XROBOT_MAIN() %s(%s)' % (call, arguments))
    else:
        lines += ['#define XROBOT_MAIN() \\', '  %s( \\' % call]
        lines += ['      %s%s \\' % (name, ',' if i+1 < len(views) else '')
                  for i, (name, _, _) in enumerate(views)]
        lines.append('  )')
    lines.append('')
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
        print('%s: %s' % (record['name'], ', '.join(record['types'])))
    return code



def generate_compile_check(module_name, modules_dir=Path('Modules'), output=Path('module_check.cpp'), template_args=None):
    modules = discover_modules(Path(modules_dir))
    module = select_module(modules, module_name)
    if not module['manifest'].standalone:
        raise ValueError(module['id'] + ' is a library, not an instantiable Module')
    interface = source_interface(module['header'])
    supplied = list(template_args or [])
    templates = template_bindings(interface, supplied)
    cpp_class = module['name'] + ('<' + ', '.join(supplied) + '>' if interface['template'] is not None else '')
    entry = {'module': module['id'], 'id': 'module_0',
             'args': initial_arguments(interface, cpp_class, templates)}
    if supplied:
        entry['template_args'] = supplied
    code = generate_xrobot_main_code({'modules': [entry]}, modules, compile_check=True)
    atomic_write(Path(output), code)
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-c', '--config', default='User/xrobot.yaml')
    parser.add_argument('-d', '--directory', default='Modules')
    parser.add_argument('-o', '--output', default='User/xrobot_main.hpp')
    parser.add_argument('--register-source', action='append', default=[])
    parser.add_argument('--lock')
    parser.add_argument('--check-module', help='Generate a non-executed constructor compile check')
    parser.add_argument('--template-arg', action='append', default=[])
    args = parser.parse_args()
    try:
        if args.check_module:
            generate_compile_check(args.check_module, args.directory, args.output, args.template_arg)
        else:
            generate(args.config, args.directory, args.output, args.register_source, args.lock)
        print('Generated ' + args.output)
    except (OSError, ValueError, TypeError, yaml.YAMLError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
