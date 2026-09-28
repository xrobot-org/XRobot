"""Load and validate XRobot application configurations.

A configuration is plain YAML whose scalars are C++ text. Loading keeps the
source line of every node so generation errors and the generated header's
``#line`` directives can point back into the YAML.
"""
import re
from pathlib import Path

import yaml

from xrobot.SourceSyntax import code_tokens

NULL_SCALARS = ('', '~', 'null', 'Null', 'NULL')
TOP_LEVEL = ('modules', 'settings', 'constexprs', 'constexpr_namespace', 'constexpr_includes')
IDENTIFIER = r'[A-Za-z_][A-Za-z_0-9]*'

# C++20 keywords and alternative tokens; an instance id or registration name
# spelled like one of these cannot become a C++ object name.
CPP_KEYWORDS = frozenset('''
alignas alignof and and_eq asm auto bitand bitor bool break case catch char char8_t
char16_t char32_t class compl concept const consteval constexpr constinit const_cast
continue co_await co_return co_yield decltype default delete do double dynamic_cast
else enum explicit export extern false float for friend goto if inline int long
mutable namespace new noexcept not not_eq nullptr operator or or_eq private protected
public register reinterpret_cast requires return short signed sizeof static
static_assert static_cast struct switch template this thread_local throw true try
typedef typeid typename union unsigned using virtual void volatile wchar_t while xor
xor_eq final override import module
'''.split())

# Macros that LibXR, the C library or the generated header define; an object
# with one of these names does not survive preprocessing.
RESERVED_MACROS = frozenset('''
ASSERT ASSERT_FROM_CALLBACK UNUSED NULL EOF assert errno offsetof
XR_REGISTER XROBOT_MAIN XR_LOG_DEBUG XR_LOG_INFO XR_LOG_PASS XR_LOG_WARN XR_LOG_ERROR
M_PI M_2PI M_1G
'''.split())

RESERVED_NAMESPACES = frozenset(('std', 'LibXR', 'xrobot_generated'))


class ConfigError(ValueError):
    """A validation error that already carries its ``<config>: <path>`` prefix."""


class Located(dict):
    """A configuration mapping that also records the YAML line of each key."""

    def __init__(self, line):
        super().__init__()
        self.line = line
        self.key_lines = {}


class LocatedList(list):
    def __init__(self, line):
        super().__init__()
        self.line = line
        self.item_lines = []


def _reject_anchors_and_tags(text, source):
    for event in yaml.parse(text, Loader=yaml.BaseLoader):
        line = event.start_mark.line + 1
        if isinstance(event, yaml.AliasEvent) or getattr(event, 'anchor', None):
            raise ConfigError('%s:%d: YAML anchors and aliases are not allowed; reference '
                              'instances by id and share values through constexprs' % (source, line))
        tag = getattr(event, 'tag', None)
        if tag not in (None, '!'):
            raise ConfigError('%s:%d: YAML tags (%s) are not allowed; write the value as C++ text'
                              % (source, line, tag))


def _construct(node, source):
    line = node.start_mark.line + 1
    if isinstance(node, yaml.ScalarNode):
        if node.style is None and node.value in NULL_SCALARS:
            return None
        return node.value
    if isinstance(node, yaml.SequenceNode):
        result = LocatedList(line)
        for child in node.value:
            result.item_lines.append(child.start_mark.line + 1)
            result.append(_construct(child, source))
        return result
    result = Located(line)
    for key_node, value_node in node.value:
        key_line = key_node.start_mark.line + 1
        if not isinstance(key_node, yaml.ScalarNode):
            raise ConfigError('%s:%d: mapping keys must be plain names' % (source, key_line))
        key = key_node.value
        if key in result:
            raise ConfigError('%s:%d: duplicate key %s' % (source, key_line, key))
        result.key_lines[key] = key_line
        result[key] = _construct(value_node, source)
    return result


def parse_yaml(text, source):
    """Parse configuration YAML into Located/LocatedList/str/None values."""
    try:
        _reject_anchors_and_tags(text, source)
        node = yaml.compose(text, Loader=yaml.BaseLoader)
    except yaml.MarkedYAMLError as error:
        mark = error.problem_mark or error.context_mark
        where = ':%d' % (mark.line + 1) if mark else ''
        raise ConfigError('%s%s: YAML syntax error: %s' % (source, where, error.problem or error)) from error
    except yaml.YAMLError as error:
        raise ConfigError('%s: YAML error: %s' % (source, error)) from error
    if node is None:
        return Located(1)
    return _construct(node, source)


def value_text(value, field):
    """Return a scalar's C++ text after the checks shared by every value."""
    if value is None:
        raise ConfigError('%s is not filled in (null, ~ and empty values mean "not filled in"; '
                          'write nullptr for a null pointer)' % field)
    if not isinstance(value, str) or not value.strip():
        raise ConfigError(field + ' requires C++ expression text')
    if value.lstrip().startswith('@'):
        raise ConfigError(field + ': use ordinary C++ expressions, not @ syntax')
    check_cpp_text(value, field)
    return value


def check_cpp_text(text, field):
    tokens = code_tokens(text)
    code = text
    for token in reversed([t for t in tokens if t.kind == 'literal']):
        code = code[:token.start] + ' ' * (token.end - token.start) + code[token.end:]
    if '//' in code or '/*' in code:
        raise ConfigError('%s: C++ comments are not allowed inside a value; use a YAML # comment' % field)
    for token in tokens:
        if token.kind == 'number' and re.fullmatch(r'0[0-9]+[uUlL]*', token.text):
            raise ConfigError('%s: %s has a leading zero, which C++ reads as octal; write the '
                              'decimal value' % (field, token.text))
    if any(ch.isdigit() and not ch.isascii() for ch in code):
        raise ConfigError('%s: non-ASCII digits are not C++ numbers' % field)


def identifier_problem(name):
    """Why ``name`` cannot be a generated C++ object name, or None."""
    if not isinstance(name, str) or not re.fullmatch(IDENTIFIER, name):
        return 'is not a C++ identifier'
    if name in CPP_KEYWORDS:
        return 'is a C++ keyword'
    if name in RESERVED_MACROS:
        return 'is a macro name'
    if name in RESERVED_NAMESPACES:
        return 'is a reserved namespace'
    if name.startswith('xr_') or name.startswith('XR_') or name.startswith('xrobot_'):
        return 'uses a prefix reserved for generated names'
    if name.startswith('__') or re.match(r'_[A-Z]', name):
        return 'is reserved by the C++ standard'
    return None


def _check_value(value, field, errors):
    if value is None:
        return  # A saved, unfilled configuration is valid; generation is not.
    if isinstance(value, dict):
        for name, child in value.items():
            if not re.fullmatch(IDENTIFIER, name):
                errors.append('%s: invalid field name %s' % (field, name))
                continue
            _check_value(child, field + '.' + name, errors)
    elif isinstance(value, list):
        for i, child in enumerate(value):
            _check_value(child, '%s[%d]' % (field, i), errors)
    else:
        try:
            value_text(value, field)
        except ConfigError as error:
            errors.append(str(error))


def validate_config(config, source='config'):
    """Validate the structure of a loaded configuration; collect every error."""
    errors = []
    if not isinstance(config, dict):
        raise ConfigError('%s: expected an application configuration mapping' % source)
    extra = [key for key in config if key not in TOP_LEVEL]
    if extra:
        errors.append('unknown top-level key(s) %s; allowed: %s' % (', '.join(extra), ', '.join(TOP_LEVEL)))
    namespace = config.get('constexpr_namespace', 'ProjectConstexpr')
    if not isinstance(namespace, str) or not re.fullmatch(IDENTIFIER + '(?:::' + IDENTIFIER + ')*', namespace):
        errors.append('constexpr_namespace must be a C++ namespace name')
    else:
        for part in namespace.split('::'):
            problem = identifier_problem(part)
            if problem:
                errors.append('constexpr_namespace: %s %s' % (part, problem))
    includes = config.get('constexpr_includes', [])
    if not isinstance(includes, list):
        errors.append('constexpr_includes must be a list of header names')
    else:
        for i, header in enumerate(includes):
            if not isinstance(header, str) or not re.fullmatch(r'<[^<>"\s]+>|[^<>"\s]+', header.strip()):
                errors.append('constexpr_includes[%d]: write a header name such as Foo.hpp or <vector>' % i)
    constants = config.get('constexprs', {})
    if not isinstance(constants, dict):
        errors.append('constexprs must be a mapping of name to {type, value}')
        constants = {}
    for name, spec in constants.items():
        problem = identifier_problem(name)
        if problem:
            errors.append('constexprs.%s %s' % (name, problem))
        if not isinstance(spec, dict) or set(spec) != {'type', 'value'}:
            errors.append('constexprs.%s requires exactly type and value' % name)
            continue
        _check_value(spec['type'], 'constexprs.%s.type' % name, errors)
        _check_value(spec['value'], 'constexprs.%s.value' % name, errors)
    entries = config.get('modules', [])
    if not isinstance(entries, list):
        errors.append('modules must be an ordered list')
        entries = []
    used = set()
    for i, entry in enumerate(entries):
        where = 'modules[%d]' % i
        if not isinstance(entry, dict):
            errors.append('%s requires module/id and ordered args/template_args' % where)
            continue
        unknown = [key for key in entry if key not in ('module', 'id', 'args', 'template_args')]
        if unknown:
            errors.append('%s: unknown key(s) %s' % (where, ', '.join(unknown)))
        for key in ('module', 'id'):
            if not isinstance(entry.get(key), str) or not entry[key]:
                errors.append('%s.%s is required' % (where, key))
        identity = entry.get('id')
        if isinstance(identity, str) and identity:
            where = identity
            problem = identifier_problem(identity)
            if problem:
                errors.append('modules[%d].id: %s %s' % (i, identity, problem))
            if identity in used:
                errors.append('modules[%d].id: duplicate instance id %s' % (i, identity))
            used.add(identity)
        values = entry.get('args', [])
        if not isinstance(values, list):
            errors.append('%s.args must be an ordered list' % where)
            values = []
        names = set()
        for j, value in enumerate(values):
            if not isinstance(value, dict) or len(value) != 1:
                errors.append('%s.args[%d] requires one named parameter' % (where, j))
                continue
            name, argument = next(iter(value.items()))
            if not re.fullmatch(IDENTIFIER, name) or name in names:
                errors.append('%s.args[%d]: invalid or duplicate parameter name %s' % (where, j, name))
            names.add(name)
            _check_value(argument, '%s.args.%s' % (where, name), errors)
        templates = entry.get('template_args', [])
        if not isinstance(templates, list):
            errors.append('%s.template_args must be an ordered list' % where)
            templates = []
        for j, value in enumerate(templates):
            if value is not None:
                _check_value(value, '%s.template_args[%d]' % (where, j), errors)
    settings = config.get('settings', {})
    if not isinstance(settings, dict) or [key for key in settings if key != 'monitor_sleep_ms']:
        errors.append('settings only accepts monitor_sleep_ms')
    else:
        sleep = settings.get('monitor_sleep_ms', '1000')
        if not isinstance(sleep, str) or not re.fullmatch(r'0|[1-9][0-9]*', sleep) or int(sleep) > 0xffffffff:
            errors.append('settings.monitor_sleep_ms must be an unsigned 32-bit decimal millisecond count')
    if errors:
        raise ConfigError('\n'.join('%s: %s' % (source, error) for error in errors))


def load_config(path, source=None):
    path = Path(path)
    source = source or path.as_posix()
    try:
        text = path.read_text(encoding='utf-8-sig')
    except UnicodeDecodeError as error:
        raise ConfigError('%s: not UTF-8 text' % source) from error
    config = parse_yaml(text, source)
    validate_config(config, source)
    return config

