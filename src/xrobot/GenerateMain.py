"""Generate User/xrobot_main.hpp: an ordered, static C++ application from one
application configuration, the BSP entry's XR_REGISTER list and the locked
Module sources.
"""
import os
import re
import stat
import tempfile
from pathlib import Path

from xr_syntax.cpp import CppDocument, identifier_occurrences

from xrobot.Config import ConfigError, identifier_problem, load_config, value_text, IDENTIFIER
from xrobot.ConstructorModel import (ValueChecker, constructor_for, convert, initializer_tree,
                                     is_dependency, qualify, template_bindings, type_shape)
from xrobot.ModuleParser import discover_modules, select_module, source_interface
from xrobot.Project import Project
from xrobot.SourceSyntax import code_tokens, close_token, split_arguments, conditional_depth
from xrobot.TypeIndex import TypeIndex, module_headers

HELPERS = '''namespace xrobot_generated {
// Implicit conversion of a configuration value to an arithmetic parameter type;
// constant conversions keep the compiler's value-change warnings.
template <typename P>
constexpr P Implicit(std::type_identity_t<P> value)
{
  return value;
}
}  // namespace xrobot_generated
'''


def atomic_write(path, text):
    """Replace ``path`` atomically, keeping an existing file's permission bits."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    data = text.encode('utf-8')
    if path.exists() and path.read_bytes() == data:
        return
    if path.exists():
        mode = stat.S_IMODE(path.stat().st_mode)
    else:
        umask = os.umask(0)
        os.umask(umask)
        mode = 0o666 & ~umask
    handle, temporary = tempfile.mkstemp(prefix=path.name + '.', suffix='.tmp', dir=str(path.parent))
    try:
        with os.fdopen(handle, 'wb') as stream:
            stream.write(data)
        os.chmod(temporary, mode)
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def caller_defined_names(items, stop):
    """Conservatively defer names declared by the BSP to the call site.

    The generated header can be included before these declarations, even at
    namespace scope. This is name collection, not C++ scope/type resolution;
    an extra template parameter is safer than an invisible type name.
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
                if items[j].text in ('{', '(', '[', '<'):
                    j = close_token(items, j)+1
                else:
                    j += 1
            names.update(t.text for t in items[i+1:j] if t.kind == 'identifier'
                         and t.text not in ('void', 'bool', 'char', 'short', 'int',
                                            'long', 'float', 'double', 'signed',
                                            'unsigned', 'const', 'volatile'))
    return names


def read_registrations(path):
    """Read the entry's XR_REGISTER(name, Type) invocations (one type per name)."""
    path = Path(path)
    text = path.read_text(encoding='utf-8-sig', errors='surrogateescape')
    label = path.name
    for number, line in enumerate(text.splitlines(), 1):
        if line.lstrip().startswith('#') and re.search(r'\bXR_REGISTER\b', line):
            raise ConfigError('%s:%d: XR_REGISTER inside a preprocessor directive is not supported'
                              % (label, number))
    document = CppDocument.parse(text, source_name=str(path))
    invocations = document.invocation_views('XR_REGISTER', template_angles=True)
    candidates = [o for o in identifier_occurrences(text)
                  if o.text == 'XR_REGISTER' and o.following == '(']
    if len(invocations) != len(candidates):
        raise ConfigError('%s: malformed XR_REGISTER invocation' % label)
    encoded = document.render_bytes()
    records, errors, names = [], [], set()
    for invocation in invocations:
        where = '%s:%s' % (label, invocation.line)
        if conditional_depth(document, 0, invocation.span.start):
            errors.append('%s: XR_REGISTER inside #if/#ifdef/#ifndef is not supported; the generator '
                          'cannot evaluate build options' % where)
            continue
        parts = [p.strip() for p in invocation.arguments]
        if len(parts) != 2:
            errors.append('%s: XR_REGISTER registers one type per name: XR_REGISTER(name, Type). To '
                          'expose the object as another type, declare a reference (e.g. '
                          '`LibXR::CAN& can1 = fdcan1;`) and register that name separately' % where)
            continue
        name, cpp_type = parts
        problem = identifier_problem(name)
        if problem:
            errors.append('%s: registration name %s %s' % (where, name, problem))
            continue
        if name in names:
            errors.append('%s: duplicate XR_REGISTER name %s' % (where, name))
            continue
        if cpp_type.endswith('&'):
            errors.append('%s: register object types, not reference types: %s' % (where, name))
            continue
        names.add(name)
        prefix = encoded[:invocation.span.start].decode('utf-8', errors='surrogateescape')
        prefix_tokens = code_tokens(prefix)
        local_names = caller_defined_names(prefix_tokens, len(prefix_tokens))
        identifiers = {t.text for t in code_tokens(cpp_type) if t.kind == 'identifier'}
        records.append({'name': name, 'type': cpp_type, 'line': invocation.line,
                        'caller_view': bool(identifiers & local_names) or 'decltype' in identifiers
                        or '*' in local_names})
    if errors:
        raise ConfigError('\n'.join(errors))
    return records


def _line_of(container, key, default):
    lines = getattr(container, 'key_lines', None)
    return lines.get(key, default) if lines else default


class _Relation:
    """Certain type facts from the loaded Module headers (D9 / G22)."""

    def __init__(self, index):
        self.index = index

    def certainly_unrelated(self, source, target):
        """True only when ``source`` can be shown not to convert to ``target``."""
        sb, _, sp, _ = type_shape(source)
        tb, _, tp, _ = type_shape(target)
        if sb == tb:
            return False
        if len(sp) != len(tp):
            return False  # pointer adaptation is decided by the generator's address-of rule
        try:
            entry = self.index.resolve(sb)
            wanted = self.index.resolve(tb)
        except ValueError:
            return False
        if entry is None or wanted is None:
            return False
        return self._derives(entry, wanted, 0) is False

    def _derives(self, entry, wanted, depth):
        if entry.path == wanted.path:
            return True
        if depth > 8:
            return None
        unknown = False
        for access, base in entry.base_spellings():
            if access != 'public':
                continue
            parent = self.index.resolve(base, entry.path[:-1])
            if parent is None:
                unknown = True
                continue
            found = self._derives(parent, wanted, depth + 1)
            if found:
                return True
            if found is None:
                unknown = True
        return None if unknown else False


class Generator:
    """Render one configuration; errors are collected, not raised one by one."""

    def __init__(self, modules, index=None):
        self.modules = modules
        self.index = index or TypeIndex.for_modules(modules)
        self.checker = ValueChecker(self.index)
        self.relation = _Relation(self.index)
        self._class_names = None

    def class_names(self):
        if self._class_names is None:
            self._class_names = self.index.global_class_names() | {m['name'] for m in self.modules.values()}
        return self._class_names

    def render(self, config, registrations, source, config_path=None, header_path=None,
               header_lines=(), compile_check=False):
        errors = []

        def fail(message):
            errors.append('%s: %s' % (source, message))

        known = {}
        for record in registrations:
            if record['name'] in self.class_names():
                fail('XR_REGISTER name %s is also a Module class name; rename the object'
                     % record['name'])
            known[record['name']] = record['type']
        entries_config = config.get('modules', [])
        ids = [entry.get('id') for entry in entries_config]
        selected, entries, monitored, earlier = {}, [], set(), {}
        for i, entry in enumerate(entries_config):
            identity = entry['id']
            try:
                result = self._instance(i, entry, ids, known, earlier, selected, compile_check)
            except ValueError as error:
                for line in str(error).splitlines():
                    fail(line if line.startswith(identity) else '%s: %s' % (identity, line))
                earlier[identity] = None
                continue
            entries.append(result)
            earlier[identity] = result['cpp_type']
            if result['monitor']:
                monitored.add(identity)
        constants = []
        namespace = config.get('constexpr_namespace', 'ProjectConstexpr')
        for name, spec in config.get('constexprs', {}).items():
            try:
                cpp_type = value_text(spec['type'], 'constexprs.%s.type' % name)
                self.checker.checks = []
                expr, typed = self.checker.render(spec['value'], 'constexprs.' + name, cpp_type, (), None)
                if expr.lstrip().startswith('{'):
                    expr = cpp_type + expr
                constants.append('inline constexpr %s %s = %s;' % (cpp_type, name, expr))
            except ValueError as error:
                fail(str(error))
        if errors:
            raise ConfigError('\n'.join(errors))
        return self._assemble(config, registrations, entries, monitored, constants, namespace,
                              config_path, header_path, header_lines, compile_check, selected)

    def _instance(self, i, entry, ids, known, earlier, selected, compile_check):
        identity = entry['id']
        if identity in known:
            raise ValueError('instance id %s is also an XR_REGISTER name' % identity)
        if identity in self.class_names():
            raise ValueError('instance id %s is also a class name in the loaded Modules; use a '
                             'lower-case id such as %s' % (identity, identity.lower()))
        module = select_module(self.modules, entry['module'])
        if not module['manifest'].standalone:
            raise ValueError('%s is a non-standalone library, not an instance' % module['id'])
        if module['name'] in selected and selected[module['name']]['id'] != module['id']:
            raise ValueError('two selected packages define global class %s; choose one implementation'
                             % module['name'])
        selected[module['name']] = module
        interface = source_interface(module['header'])
        template_args = [value_text(v, '%s.template_args[%d]' % (identity, j))
                         for j, v in enumerate(entry.get('template_args', []))]
        cpp_type = module['name']
        if template_args or interface['template'] is not None:
            cpp_type += '<' + ', '.join(template_args) + '>'
        templates = template_bindings(interface, template_args)
        named_values = entry.get('args', [])
        visible = dict(known)
        visible.update({k: v for k, v in earlier.items() if v is not None})
        later = set(ids[i:])
        ctor = constructor_for(interface, named_values, visible, cpp_type, templates)
        args_lines = getattr(entry.get('args'), 'item_lines', None) or []
        declarations, arguments, problems = [], [], []
        for j, (p, item) in enumerate(zip(ctor['arguments'], named_values)):
            value = next(iter(item.values()))
            line = args_lines[j] if j < len(args_lines) else getattr(entry, 'line', 0)
            try:
                decl, arg = self._argument(identity, p, value, interface, cpp_type, templates,
                                           visible, later, earlier, compile_check)
            except ValueError as error:
                problems.append(str(error))
                continue
            declarations.append((line, decl))
            arguments.append((line, arg))
        located = self.index.resolve(module['name'])
        monitor = self.index.provides_monitor(located) if located is not None else None
        if monitor is None:
            problems.append('%s: cannot tell whether a public base class provides OnMonitor; its '
                            'base is not defined in the loaded Module headers' % identity)
        if problems:
            raise ValueError('\n'.join(problems))
        return {'id': identity, 'cpp_type': cpp_type, 'arguments': arguments, 'index': i,
                'declarations': declarations, 'monitor': monitor, 'line': getattr(entry, 'line', 0)}

    def _argument(self, identity, p, value, interface, cpp_class, templates, visible, later, earlier,
                  compile_check):
        field = '%s.args.%s' % (identity, p['name'])
        typ = qualify(p['type'], interface, cpp_class, templates)
        target = qualify(p['type'], interface, cpp_class, templates, True)
        _, _, pointers, reference = type_shape(target)
        checks = []
        if value is None and compile_check and p['default'] is None:
            raw = '*static_cast<std::remove_reference_t<%s>*>(xr_ci_null)' % typ
            return [], ('static_cast<%s>(%s)' % (typ, raw) if typ.rstrip().endswith('&&') else raw)
        default = None
        if p['default'] is not None:
            default = initializer_tree(qualify(p['default'], interface, cpp_class, templates), target)
        self.checker.checks = checks
        if isinstance(value, str):
            text = value_text(value, field)
            name = text.strip()
            reference_name = name[1:].strip() if name.startswith('&') else name
            if re.fullmatch(IDENTIFIER, reference_name) and (reference_name == name or len(pointers) == 1):
                if reference_name in later:
                    raise ValueError('%s: %s is constructed at or after %s; instances are constructed in '
                                     'list order' % (field, reference_name, identity))
                if reference_name in earlier and earlier[reference_name] is None:
                    raise ValueError('%s: %s has errors of its own' % (field, reference_name))
                if is_dependency(p) and reference_name not in visible and name != 'nullptr':
                    raise ValueError('%s: %s is neither an XR_REGISTER name nor an earlier instance id; '
                                     'candidates of type %s: %s' % (
                                         field, reference_name, target,
                                         ', '.join(self._candidates(target, visible)) or 'none'))
                if reference_name in visible:
                    source = visible[reference_name]
                    expression = name
                    if name.startswith('&'):
                        source += '*'
                    elif len(pointers) == 1 and not type_shape(source)[2] and reference != '&':
                        # A bare name bound to a pointer parameter passes the object's address.
                        expression = 'std::addressof(%s)' % name
                        source += '*'
                    if self.relation.certainly_unrelated(source, target):
                        raise ValueError('%s: %s is a %s, which does not convert to %s' % (
                            field, reference_name, visible[reference_name], target))
                    if type_shape(source)[0] == type_shape(target)[0] and \
                            len(type_shape(source)[2]) == len(pointers):
                        return checks, expression  # the same type binds directly
                    if reference:
                        convert(expression, typ, False, checks, field)
                        return checks, 'static_cast<%s>(%s)' % (typ, expression)
                    expr, _ = convert(expression, typ, False, checks, field)
                    return checks, expr
            expr, typed = self.checker.render(text, field, target, (), default)
        else:
            expr, typed = self.checker.render(value, field, target, (), default)
        exact = typed or isinstance(value, (dict, list))
        if reference or 'std::initializer_list<' in target.replace(' ', ''):
            # Config temporaries and initializer-list backing arrays need static lifetime.
            storage = 'xr_arg_%s_%s' % (identity, p['name'])
            checks.append('static %s %s =\n      %s\n  ;' % (typ, storage, expr))
            return checks, ('static_cast<%s>(%s)' % (typ, storage) if typ.rstrip().endswith('&&') else storage)
        expr, _ = convert(expr, typ, exact, checks, field)
        return checks, expr

    def _candidates(self, target, visible):
        tb = type_shape(target)[0]
        return [name for name, cpp_type in visible.items() if type_shape(cpp_type)[0] == tb]

    def _assemble(self, config, registrations, entries, monitored, constants, namespace, config_path,
                  header_path, header_lines, compile_check, selected):
        used = set()
        for entry in entries:
            for expression in [entry['cpp_type']] + [a for _, a in entry['arguments']] + \
                    [d for _, ds in entry['declarations'] for d in ds]:
                used.update(t.text for t in code_tokens(expression) if t.kind == 'identifier')
        views = [r for r in registrations if r['name'] in used]
        template_names = {t.text for r in views for t in code_tokens(r['type']) if t.kind == 'identifier'}
        template_names |= set(selected) | {e['id'] for e in entries} | {r['name'] for r in views}
        local_templates, parameters, actual_types = [], [], []
        for i, record in enumerate(views):
            typ = record['type']
            if record['caller_view']:
                parameter_type = 'XrViewType%d' % i
                while parameter_type in template_names:
                    parameter_type += '_'
                template_names.add(parameter_type)
                local_templates.append('typename ' + parameter_type)
                actual_types.append(typ)
                typ = parameter_type
            declaration = ('std::add_lvalue_reference_t<%s>' % typ
                           if any(t.text in ('(', '[') for t in code_tokens(typ)) else typ + '&')
            parameters.append('%s %s' % (declaration, record['name']))
        lines = ['#pragma once'] + list(header_lines)
        lines += ['', '#include <memory>', '#include <type_traits>', '#include <utility>',
                  '#include "libxr.hpp"', '#include "thread.hpp"']
        lines += ['#include "%s.hpp"' % name for name in selected]
        for header in config.get('constexpr_includes', []):
            header = header.strip()
            lines.append('#include %s' % (header if header.startswith('<') else '"%s"' % header))
        if compile_check:
            lines = lines[2:]
        lines += ['', HELPERS]
        if constants:
            lines += ['namespace %s {' % namespace] + constants + ['}  // namespace %s' % namespace, '']
        back = object()  # placeholder for "#line back into this header"
        directives = config_path is not None and header_path is not None and not compile_check
        cfg = Path(os.path.abspath(config_path)).as_posix() if directives else None

        def at(line):
            if directives and line:
                lines.append('#line %d "%s"' % (line, cfg))

        if compile_check:
            lines += ['namespace xrobot_generated {', 'void XRobotCompileCheck() {',
                      '  // Compilation only: this function must never be invoked.',
                      '  [[maybe_unused]] static void* xr_ci_null = static_cast<void*>(nullptr);']
        else:
            if local_templates:
                lines.append('template <%s>' % ', '.join(local_templates))
            signature = '[[noreturn]] static inline void XRobotMain('
            if parameters:
                lines += [signature, '    ' + ',\n    '.join(parameters) + ')', '{']
            else:
                lines += [signature + ')', '{']
        for entry in entries:
            lines.append('  // modules[%d]: %s' % (entry['index'], entry['id']))
            for line, declarations in entry['declarations']:
                if declarations:
                    at(line)
                    lines.extend('  ' + d for d in declarations)
            at(entry['line'])
            if entry['arguments']:
                lines.append('  static %s %s(' % (entry['cpp_type'], entry['id']))
                for k, (line, argument) in enumerate(entry['arguments']):
                    at(line)
                    lines.append('      %s%s' % (', ' if k else '', argument))
                lines.append('  );')
            else:
                lines.append('  static %s %s;' % (entry['cpp_type'], entry['id']))
            if directives:
                lines.append(back)
        lines += ['  static_assert(std::is_void_v<decltype(%s.OnMonitor())>, "%s.OnMonitor() must return void");'
                  % (e['id'], e['id']) for e in entries if e['id'] in monitored]
        if compile_check:
            lines += ['  %s.OnMonitor();' % e['id'] for e in entries if e['id'] in monitored]
            lines += ['}', '}  // namespace xrobot_generated', '']
            return '\n'.join(lines)
        lines += ['  for (;;)', '  {']
        lines += ['    %s.OnMonitor();' % e['id'] for e in entries if e['id'] in monitored]
        lines += ['    LibXR::Thread::Sleep(%s);' % config.get('settings', {}).get('monitor_sleep_ms', '1000'),
                  '  }', '}', '']
        lines += ['// XR_REGISTER marks names for the generator and uses the object, so a',
                  '// registration the selected product does not consume is not an unused',
                  '// variable; types are checked where XRobotMain binds them.',
                  '#define XR_REGISTER(name, ...) static_cast<void>(name)']
        call = '::XRobotMain' + ('<%s>' % ', '.join(actual_types) if actual_types else '')
        arguments = ', '.join(r['name'] for r in views)
        if len(call) + len(arguments) < 72:
            lines.append('#define XROBOT_MAIN() %s(%s)' % (call, arguments))
        else:
            lines += ['#define XROBOT_MAIN() \\', '  %s( \\' % call]
            lines += ['      %s%s \\' % (r['name'], ',' if i + 1 < len(views) else '')
                      for i, r in enumerate(views)]
            lines.append('  )')
        lines.append('')
        if directives:
            own = Path(os.path.abspath(header_path)).as_posix()
            result, number = [], 1  # number: line number of the next emitted line
            for line in lines:
                if line is back:
                    line = '#line %d "%s"' % (number + 1, own)
                result.append(line)
                number += line.count('\n') + 1
            lines = result
        return '\n'.join(lines)


def generate_code(project, config_path, modules, registrations, index=None):
    """Render the header for one configuration without writing it."""
    config_path = Path(config_path)
    source = project.relative(config_path)
    config = load_config(config_path, source)
    depends = ([project.lock] if project.lock.is_file() else []) + [project.entry()] + \
        sorted(set(module_headers(modules)))
    generator = Generator(modules, index)
    return generator.render(config, registrations, source, config_path, project.header,
                            project.header_lines(config_path, depends))


def load_modules(project):
    return discover_modules(project.modules_dir, project.lock)


def generate(project, config_path=None):
    """Generate User/xrobot_main.hpp for ``config_path`` (default: the selected product)."""
    config_path = Path(config_path) if config_path else project.selected_config()
    if not config_path.is_file():
        raise ConfigError('%s does not exist' % project.relative(config_path))
    modules = load_modules(project)
    registrations = read_registrations(project.entry())
    code = generate_code(project, config_path, modules, registrations)
    atomic_write(project.header, code)
    # Unchanged content is not rewritten; still mark the header as generated
    # after its inputs so the build's freshness check accepts it.
    os.utime(project.header)
    return code


def validate_all(project, modules=None, index=None):
    """Check every application configuration of the BSP; collect all errors (G32)."""
    modules = modules if modules is not None else load_modules(project)
    index = index or TypeIndex.for_modules(modules)
    registrations = read_registrations(project.entry())
    errors = []
    for config_path in project.configs():
        try:
            generate_code(project, config_path, modules, registrations, index)
        except ValueError as error:
            errors.append(str(error))
    if errors:
        raise ConfigError('\n'.join(errors))
    return len(project.configs())


def generate_compile_check(module_name, modules, output, template_args=None):
    """A never-executed constructor call with void* placeholders (module CI probe)."""
    from xrobot.ConfigEdit import seed_arguments
    module = select_module(modules, module_name)
    if not module['manifest'].standalone:
        raise ConfigError(module['id'] + ' is a library, not an instantiable Module')
    interface = source_interface(module['header'])
    supplied = list(template_args or [])
    templates = template_bindings(interface, supplied)
    cpp_class = module['name'] + ('<' + ', '.join(supplied) + '>' if interface['template'] is not None else '')
    generator = Generator(modules)
    entry = {'module': module['id'], 'id': 'module_0',
             'args': seed_arguments(interface, cpp_class, templates, generator.index)}
    if supplied:
        entry['template_args'] = supplied
    code = generator.render({'modules': [entry]}, [], module['id'], compile_check=True)
    atomic_write(Path(output), code)
    return code
