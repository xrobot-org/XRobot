"""Edit application configurations and modules.yaml without losing comments.

Instances are edited as text blocks: an instance owns its ``- module:`` item
and the comment lines directly above it, so adding, removing or renaming an
instance moves its comments with it. Values inside one instance are edited
through a comment-preserving YAML round trip of that instance only. Every
write uses the canonical layout that ``xrobot format`` enforces.
"""
import hashlib
import io
import json
import re
from pathlib import Path

import yaml
from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq

from xrobot.Config import ConfigError, identifier_problem, load_config, parse_yaml
from xrobot.ConstructorModel import (compliant_constructors, initializer_tree, is_dependency,
                                     qualify, replace_names, template_bindings)
from xrobot.GenerateMain import atomic_write
from xrobot.ModuleParser import select_module, source_interface
from xrobot.SourceSyntax import code_tokens


def _yaml():
    document = YAML()
    document.preserve_quotes = True
    document.width = 4096
    document.indent(mapping=2, sequence=4, offset=2)
    return document


def dump_text(data):
    stream = io.StringIO()
    _yaml().dump(data, stream)
    return stream.getvalue()


def canonical_text(text):
    """The canonical layout of a YAML document (comments and quoting kept)."""
    if text.startswith('﻿'):
        text = text[1:]
    text = text.replace('\r\n', '\n')
    data = _yaml().load(text)
    if data is None:
        return text if text.endswith('\n') or not text else text + '\n'
    return dump_text(data)


def file_hash(path):
    return hashlib.sha256(Path(path).read_bytes().replace(b'\r\n', b'\n')).hexdigest()


# -- instance blocks -------------------------------------------------------------

class Blocks:
    """Line spans of the ``modules`` list items of one configuration file."""

    def __init__(self, text, source):
        if text.startswith('﻿'):
            text = text[1:]
        self.text = text.replace('\r\n', '\n')
        self.lines = self.text.split('\n')
        root = yaml.compose(self.text, Loader=yaml.BaseLoader) if self.text.strip() else None
        self.items = []  # (comment_start, start, end) line indices, end exclusive
        self.sequence_end = None
        self.item_indent = None
        if root is None or not isinstance(root, yaml.MappingNode):
            return
        key = next((k for k, v in root.value if k.value == 'modules'), None)
        node = next((v for k, v in root.value if k.value == 'modules'), None)
        if node is None or not isinstance(node, yaml.SequenceNode) or node.flow_style:
            return
        starts = [child.start_mark.line for child in node.value]
        end = self._block_end(node)
        self.sequence_end = end
        for k, start in enumerate(starts):
            item_end = starts[k + 1] if k + 1 < len(starts) else end
            comment_start = start
            while comment_start > 0 and self._is_comment_or_blank(comment_start - 1) and \
                    (k == 0 or comment_start - 1 >= starts[k - 1]):
                comment_start -= 1
            if k == 0:
                comment_start = max(comment_start, key.start_mark.line + 1)
            self.items.append([comment_start, start, item_end])
        # An item's trailing comment/blank lines belong to the next item.
        for k in range(len(self.items) - 1):
            self.items[k][2] = self.items[k + 1][0]
        if starts:
            line = self.lines[starts[0]]
            self.item_indent = len(line) - len(line.lstrip(' '))

    def _is_comment_or_blank(self, index):
        stripped = self.lines[index].strip()
        return not stripped or stripped.startswith('#')

    def _block_end(self, node):
        end = node.end_mark.line
        # Trailing comment/blank lines at the end of the list stay with the file.
        while end > node.start_mark.line and end - 1 < len(self.lines) and \
                self._is_comment_or_blank(end - 1):
            end -= 1
        return end

    def item_text(self, k):
        _, start, end = self.items[k]
        return '\n'.join(self.lines[start:end])

    def replace(self, k, new_lines, keep_comments=True):
        comment_start, start, end = self.items[k]
        begin = start if keep_comments else comment_start
        lines = self.lines[:begin] + new_lines + self.lines[end:]
        return '\n'.join(lines)

    def remove(self, k):
        comment_start, _, end = self.items[k]
        return '\n'.join(self.lines[:comment_start] + self.lines[end:])

    def append(self, new_lines):
        end = self.sequence_end if self.sequence_end is not None else len(self.lines)
        return '\n'.join(self.lines[:end] + new_lines + self.lines[end:])


def _render_item(item, indent):
    """Render one instance mapping as a list item at ``indent`` spaces."""
    text = dump_text([item])
    lines = text.rstrip('\n').split('\n')
    base = len(lines[0]) - len(lines[0].lstrip(' '))
    return [' ' * indent + line[base:] if line.strip() else '' for line in lines]


def _load_item(block_text):
    """Load one instance block (a one-item list) for a round-trip edit."""
    lines = block_text.split('\n')
    indent = min(len(l) - len(l.lstrip(' ')) for l in lines if l.strip())
    data = _yaml().load('\n'.join(l[indent:] for l in lines))
    if not isinstance(data, CommentedSeq) or len(data) != 1:
        raise ConfigError('cannot edit this instance: its YAML block is not a single list item')
    return data[0], indent


class ConfigFile:
    def __init__(self, path, source=None):
        self.path = Path(path)
        self.source = source or self.path.as_posix()
        self.text = self.path.read_text(encoding='utf-8-sig') if self.path.exists() else 'modules: []\n'
        self.config = parse_yaml(self.text, self.source)

    def blocks(self):
        return Blocks(self.text, self.source)

    def index_of(self, instance_id):
        for k, entry in enumerate(self.config.get('modules') or []):
            if isinstance(entry, dict) and entry.get('id') == instance_id:
                return k
        raise ConfigError('%s: no instance with id %s' % (self.source, instance_id))

    def write(self, text, check=True):
        text = canonical_text(text)
        config = parse_yaml(text, self.source)
        if check:
            from xrobot.Config import validate_config
            validate_config(config, self.source)
        atomic_write(self.path, text)
        self.text, self.config = text, config


def _path_tokens(path):
    tokens = re.findall(r'[A-Za-z_][A-Za-z_0-9]*|\[\d+\]', path)
    if not tokens or ''.join(t if t.startswith('[') else '.' + t for t in tokens).lstrip('.') != path:
        raise ConfigError('invalid path %s; use id, template_args[n], args.<param>.<field>..., [n]' % path)
    return tokens


def _set_path(item, path, value):
    """Set ``value`` at a dotted path inside one instance mapping."""
    tokens = _path_tokens(path)
    node = item
    for position, token in enumerate(tokens):
        last = position == len(tokens) - 1
        if token.startswith('['):
            index = int(token[1:-1])
            if not isinstance(node, list) or index >= len(node):
                raise ConfigError('path %s: no element %d' % (path, index))
            if last:
                node[index] = value
                return
            node = node[index]
            continue
        if node is item and token == 'args':
            if last:
                raise ConfigError('set a single argument: args.<param>')
            name = tokens[position + 1]
            args = item.get('args') or []
            for entry in args:
                if isinstance(entry, dict) and name in entry:
                    if position + 1 == len(tokens) - 1:
                        entry[name] = value
                        return
                    node = entry[name]
                    break
            else:
                raise ConfigError('path %s: no argument %s' % (path, name))
            rest = tokens[position + 2:]
            return _set_path_tail(node, rest, value, path)
        if not isinstance(node, dict) or token not in node:
            raise ConfigError('path %s: no key %s' % (path, token))
        if last:
            node[token] = value
            return
        node = node[token]


def _set_path_tail(node, tokens, value, path):
    for position, token in enumerate(tokens):
        last = position == len(tokens) - 1
        if token.startswith('['):
            index = int(token[1:-1])
            if not isinstance(node, list) or index >= len(node):
                raise ConfigError('path %s: no element %d' % (path, index))
            if last:
                node[index] = value
                return
            node = node[index]
        else:
            if not isinstance(node, dict) or token not in node:
                raise ConfigError('path %s: no key %s' % (path, token))
            if last:
                node[token] = value
                return
            node = node[token]


def _to_yaml_value(value):
    """JSON value -> configuration value (numbers and booleans become C++ text)."""
    if isinstance(value, bool):
        return 'true' if value else 'false'
    if isinstance(value, (int, float)):
        return str(value)
    if isinstance(value, dict):
        result = CommentedMap()
        for key, child in value.items():
            result[key] = _to_yaml_value(child)
        return result
    if isinstance(value, list):
        return CommentedSeq(_to_yaml_value(v) for v in value)
    return value


def set_value(config_path, instance_id, path, value, if_match=None, source=None):
    """Replace one node of one instance (D7); other text is untouched."""
    config = ConfigFile(config_path, source)
    if if_match is not None and file_hash(config.path) != if_match:
        raise ConfigError('%s changed since it was read; reload and retry' % config.source)
    k = config.index_of(instance_id)
    new_value = _to_yaml_value(value)
    if isinstance(new_value, str):
        text = _replace_scalar(config.text, k, path, new_value)
        if text is not None:
            config.write(text)
            return
    blocks = config.blocks()
    item, indent = _load_item(blocks.item_text(k))
    _set_path(item, path, new_value)
    config.write(blocks.replace(k, _render_item(item, indent)))


def _scalar_text(value, style):
    """Render ``value`` as a one-line YAML scalar, keeping the old quoting style."""
    if style == '"':
        return json.dumps(value, ensure_ascii=False)
    if style is None and value not in ('', '~', 'null', 'Null', 'NULL') and '\n' not in value:
        try:
            node = yaml.compose('k: ' + value, Loader=yaml.BaseLoader)
            child = node.value[0][1]
            if isinstance(child, yaml.ScalarNode) and child.style is None and child.value == value:
                return value
        except yaml.YAMLError:
            pass
    return "'" + value.replace("'", "''") + "'"


def _replace_scalar(text, index, path, value):
    """Replace a one-line scalar in place; None when the path is not such a scalar."""
    body = text[1:] if text.startswith('\ufeff') else text
    body = body.replace('\r\n', '\n')
    root = yaml.compose(body, Loader=yaml.BaseLoader)
    try:
        node = dict((k.value, v) for k, v in root.value)['modules'].value[index]
    except (AttributeError, KeyError, IndexError, TypeError):
        return None
    tokens = _path_tokens(path)
    position = 0
    while position < len(tokens):
        token = tokens[position]
        if token.startswith('['):
            if not isinstance(node, yaml.SequenceNode) or int(token[1:-1]) >= len(node.value):
                return None
            node = node.value[int(token[1:-1])]
        elif isinstance(node, yaml.MappingNode):
            children = dict((k.value, v) for k, v in node.value)
            if token not in children:
                return None
            node = children[token]
            if token == 'args' and position == 0 and position + 1 < len(tokens):
                name = tokens[position + 1]
                match = [v for item in node.value if isinstance(item, yaml.MappingNode)
                         for k, v in item.value if k.value == name]
                if len(match) != 1:
                    return None
                node = match[0]
                position += 1
        else:
            return None
        position += 1
    if not isinstance(node, yaml.ScalarNode) or node.style not in (None, "'", '"') or \
            node.start_mark.line != node.end_mark.line or (node.style is None and not node.value):
        return None
    lines = body.split('\n')
    line = lines[node.start_mark.line]
    lines[node.start_mark.line] = line[:node.start_mark.column] + _scalar_text(value, node.style) + \
        line[node.end_mark.column:]
    return '\n'.join(lines)


def remove_instance(config_path, instance_id, source=None):
    config = ConfigFile(config_path, source)
    k = config.index_of(instance_id)
    users = references_to(config.config, instance_id)
    if users:
        raise ConfigError('%s: %s is still used by %s; change those values first'
                          % (config.source, instance_id, ', '.join(users)))
    text = config.blocks().remove(k)
    if parse_yaml(text, config.source).get('modules') is None:
        text = re.sub(r'^modules:[ \t]*$', 'modules: []', text, count=1, flags=re.M)
    config.write(text)


def _values(node):
    if isinstance(node, dict):
        for child in node.values():
            yield from _values(child)
    elif isinstance(node, list):
        for child in node:
            yield from _values(child)
    elif isinstance(node, str):
        yield node


def _mentions(text, name):
    items = code_tokens(text)
    for i, token in enumerate(items):
        if token.kind == 'identifier' and token.text == name and \
                (not i or items[i - 1].text not in ('.', '->', '::')):
            return True
    return False


def references_to(config, instance_id):
    users = []
    for entry in config.get('modules') or []:
        if not isinstance(entry, dict) or entry.get('id') == instance_id:
            continue
        values = list(_values(entry.get('args') or [])) + list(_values(entry.get('template_args') or []))
        if any(_mentions(v, instance_id) for v in values):
            users.append(entry.get('id'))
    return users


def rename_instance(config_path, instance_id, new_id, source=None):
    """Rename an instance and every reference to it in the same configuration."""
    problem = identifier_problem(new_id)
    if problem:
        raise ConfigError('%s %s' % (new_id, problem))
    config = ConfigFile(config_path, source)
    if any(isinstance(e, dict) and e.get('id') == new_id for e in config.config.get('modules') or []):
        raise ConfigError('%s: instance id %s already exists' % (config.source, new_id))
    target = config.index_of(instance_id)
    text = config.text
    blocks = Blocks(text, config.source)
    for k in reversed(range(len(blocks.items))):
        item, indent = _load_item(blocks.item_text(k))
        changed = False
        if k == target:
            item['id'] = new_id
            changed = True
        for key in ('args', 'template_args'):
            if key in item and _rename_values(item[key], instance_id, new_id):
                changed = True
        if changed:
            text = blocks.replace(k, _render_item(item, indent))
            blocks = Blocks(text, config.source)
    config.write(text)


def _rename_values(node, old, new):
    changed = False
    if isinstance(node, dict):
        for key in list(node):
            child = node[key]
            if isinstance(child, str):
                if _mentions(child, old):
                    node[key] = _requote(child, replace_names(str(child), {old: new}))
                    changed = True
            elif _rename_values(child, old, new):
                changed = True
    elif isinstance(node, list):
        for i, child in enumerate(node):
            if isinstance(child, str):
                if _mentions(child, old):
                    node[i] = _requote(child, replace_names(str(child), {old: new}))
                    changed = True
            elif _rename_values(child, old, new):
                changed = True
    return changed


def _requote(original, text):
    """Keep ruamel's quoting style of ``original`` for the replaced text."""
    cls = type(original)
    try:
        return cls(text)
    except TypeError:
        return text


# -- seeding (instance add, module compile probe) ------------------------------

def _field_default(entry, name, index, spelled):
    text = entry.layout().field_defaults.get(name)
    return index.qualify_in(text, entry, spelled) if text is not None else None


def seed_value(default, target, index, scope=()):
    """Seed a configuration value from a source default (G5 / C5 corrected).

    A located aggregate whose default is ``{}`` or a designated initializer is
    written as a complete mapping (fields in order, each with its default
    member initializer, else ``{}``); every other default (factory calls,
    constants, casts) is kept verbatim.
    """
    if default is None:
        return None
    tree = initializer_tree(default, target) if isinstance(default, str) else default
    entry = index.resolve(target, scope) if index is not None and target else None
    if entry is not None and entry.is_aggregate() and entry.mapping_problem() is None and \
            (tree == [] or isinstance(tree, dict)):
        spelled = re.sub(r'^\s*(const\s+)?', '', target).rstrip('&* ').replace(' const', '')
        result = CommentedMap()
        given = tree if isinstance(tree, dict) else {}
        for name, cpp_type, _ in entry.fields():
            field_type = index.qualify_in(cpp_type, entry, spelled)
            if name in given:
                value = given[name]
            else:
                value = _field_default(entry, name, index, spelled)
            if isinstance(value, (dict, list)) or value is None or isinstance(value, str):
                child = seed_value(value if value is not None else '{}', field_type, index, entry.path)
            else:
                child = value
            result[name] = child if child is not None else '{}'
        return result
    if isinstance(tree, dict):
        result = CommentedMap()
        for name, value in tree.items():
            result[name] = seed_value(value, None, None) if not isinstance(value, str) else value
        return result
    if isinstance(tree, list):
        return default if isinstance(default, str) else CommentedSeq(tree)
    return default


def seed_arguments(interface, cpp_class, templates, index):
    ctor = compliant_constructors(interface, cpp_class, templates)[0]
    result = []
    for p in ctor['arguments']:
        if p['default'] is None:
            value = None
        else:
            default = qualify(p['default'], interface, cpp_class, templates)
            target = qualify(p['type'], interface, cpp_class, templates, True)
            value = seed_value(default, target, index)
        result.append({p['name']: value})
    return result


def next_instance_id(modules, base_name):
    prefix = base_name.lower() + '_'
    names = {entry.get('id', '') for entry in modules if isinstance(entry, dict)}
    number = 0
    while prefix + str(number) in names:
        number += 1
    return prefix + str(number)


def add_instance(config_path, module_name, modules, index, instance_id=None, source=None):
    config = ConfigFile(config_path, source)
    module = select_module(modules, module_name)
    if not module['manifest'].standalone:
        raise ConfigError('%s is a library dependency, not a Module instance' % module['id'])
    interface = source_interface(module['header'])
    identity = instance_id or next_instance_id(config.config.get('modules') or [], module['name'])
    problem = identifier_problem(identity)
    if problem:
        raise ConfigError('instance id %s %s' % (identity, problem))
    item = CommentedMap()
    item['module'] = module['id']
    item['id'] = identity
    template_values = [p['default'] for p in interface['template_parameters']]
    if template_values:
        item['template_args'] = CommentedSeq(template_values)
    if all(v is not None for v in template_values):
        templates = template_bindings(interface, template_values)
        cpp_class = module['name'] + ('<' + ', '.join(template_values) + '>' if template_values else '')
        arguments = seed_arguments(interface, cpp_class, templates, index)
        if arguments:
            args = CommentedSeq()
            for argument in arguments:
                mapping = CommentedMap()
                for key, value in argument.items():
                    mapping[key] = value
                args.append(mapping)
            item['args'] = args
    blocks = config.blocks()
    indent = blocks.item_indent if blocks.item_indent is not None else 2
    if blocks.sequence_end is None:
        text = config.text.rstrip('\n')
        text = re.sub(r'^modules:\s*\[\]\s*$', 'modules:', text, flags=re.M)
        if not re.search(r'^modules:', text, flags=re.M):
            text = 'modules:\n' + text
        lines = text.split('\n')
        at = next(i for i, l in enumerate(lines) if l.startswith('modules:')) + 1
        new_text = '\n'.join(lines[:at] + _render_item(item, indent) + lines[at:]) + '\n'
    else:
        new_text = blocks.append(_render_item(item, indent))
    config.write(new_text, check=False)
    return identity


# -- field sync (G5) -------------------------------------------------------------

def sync_config(config_path, modules, index, source=None):
    """Add new fields/defaulted parameters and drop removed fields; return a diff."""
    import difflib
    from xrobot.ConstructorModel import constructor_for
    config = ConfigFile(config_path, source)
    before = config.text
    text = before
    blocks = Blocks(text, config.source)
    entries = config.config.get('modules') or []
    for k in reversed(range(len(blocks.items))):
        entry = entries[k] if k < len(entries) else None
        if not isinstance(entry, dict) or not entry.get('module'):
            continue
        try:
            module = select_module(modules, entry['module'])
            interface = source_interface(module['header'])
        except ValueError:
            continue
        template_args = [str(v) for v in entry.get('template_args') or [] if v is not None]
        if len(template_args) != len(entry.get('template_args') or []):
            continue
        cpp_class = module['name'] + ('<' + ', '.join(template_args) + '>'
                                      if interface['template'] is not None else '')
        try:
            templates = template_bindings(interface, template_args)
        except ValueError:
            continue
        item, indent = _load_item(blocks.item_text(k))
        if _sync_item(item, interface, cpp_class, templates, index):
            text = blocks.replace(k, _render_item(item, indent))
            blocks = Blocks(text, config.source)
    if text != before:
        config.write(text, check=False)
    return ''.join(difflib.unified_diff(before.splitlines(True), canonical_text(text).splitlines(True),
                                        config.source, config.source))


def _sync_item(item, interface, cpp_class, templates, index):
    args = item.get('args')
    names = [next(iter(a)) for a in args or [] if isinstance(a, dict) and a]
    ctors = compliant_constructors(interface, cpp_class, templates)
    exact = [c for c in ctors if [p['name'] for p in c['arguments']] == names]
    changed = False
    if not exact:
        # New trailing parameters with defaults: extend when exactly one constructor
        # starts with the configured names.
        extended = [c for c in ctors if [p['name'] for p in c['arguments']][:len(names)] == names
                    and all(p['default'] is not None for p in c['arguments'][len(names):])]
        if len(extended) != 1:
            return False
        ctor = extended[0]
        if args is None:
            args = CommentedSeq()
            item['args'] = args
        for p in ctor['arguments'][len(names):]:
            mapping = CommentedMap()
            default = qualify(p['default'], interface, cpp_class, templates)
            target = qualify(p['type'], interface, cpp_class, templates, True)
            mapping[p['name']] = seed_value(default, target, index)
            args.append(mapping)
        changed = True
    else:
        ctor = exact[0]
    for p, argument in zip(ctor['arguments'], args or []):
        value = argument.get(p['name'])
        if isinstance(value, dict):
            target = qualify(p['type'], interface, cpp_class, templates, True)
            if _sync_mapping(value, target, index, ()):
                changed = True
    return changed


def _sync_mapping(value, target, index, scope):
    entry = index.resolve(target, scope) if target else None
    if entry is None or not entry.is_aggregate() or entry.mapping_problem() is not None:
        return False
    spelled = re.sub(r'^\s*(const\s+)?', '', target).rstrip('&* ')
    fields = entry.fields()
    wanted = [name for name, _, _ in fields]
    changed = False
    for key in list(value):
        if key not in wanted:
            del value[key]
            changed = True
    for position, (name, cpp_type, _) in enumerate(fields):
        field_type = index.qualify_in(cpp_type, entry, spelled)
        if name not in value:
            default = _field_default(entry, name, index, spelled)
            value.insert(position, name, seed_value(default if default is not None else '{}',
                                                    field_type, index, entry.path) or '{}')
            changed = True
        elif isinstance(value[name], dict):
            if _sync_mapping(value[name], field_type, index, entry.path):
                changed = True
    order = [k for k in wanted if k in value]
    if list(value) != order:
        items = [(k, value[k]) for k in order]
        for k in list(value):
            del value[k]
        for k, v in items:
            value[k] = v
        changed = True
    return changed


# -- modules.yaml ----------------------------------------------------------------

def _load_document(path, default):
    path = Path(path)
    if not path.exists():
        return default
    data = _yaml().load(path.read_text(encoding='utf-8-sig'))
    return data if data is not None else default


def add_module(modules_yaml, request_text):
    from xrobot.InitModule import request
    parsed = request(request_text, canonical=True)
    data = _load_document(modules_yaml, CommentedMap(modules=CommentedSeq()))
    entries = data.setdefault('modules', CommentedSeq())
    if any(request(e)['id'].casefold() == parsed['id'].casefold() for e in entries):
        raise ConfigError('%s is already requested in %s' % (parsed['id'], modules_yaml))
    entries.append(request_text if '@' in request_text else request_text + '@same-or-dev')
    atomic_write(Path(modules_yaml), dump_text(data))


def remove_module(modules_yaml, identity):
    from xrobot.InitModule import request
    data = _load_document(modules_yaml, None)
    entries = (data or {}).get('modules') or []
    matches = [i for i, e in enumerate(entries) if request(e)['id'].casefold() == identity.casefold()]
    if not matches:
        raise ConfigError('%s is not requested in %s' % (identity, modules_yaml))
    del entries[matches[0]]
    atomic_write(Path(modules_yaml), dump_text(data))


def format_files(paths, check=False):
    """Rewrite files in the canonical layout; with ``check`` only report them."""
    changed = []
    for path in paths:
        path = Path(path)
        original = path.read_bytes().decode('utf-8')
        formatted = canonical_text(original)
        if formatted != original:
            changed.append(path)
            if not check:
                atomic_write(path, formatted)
    return changed


def parse_json_value(text):
    try:
        return json.loads(text)
    except json.JSONDecodeError as error:
        raise ConfigError('value must be JSON (e.g. "\\"LED_B\\"", 1000, {"a": "1"}): %s' % error)
