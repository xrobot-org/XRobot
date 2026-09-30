"""编辑应用配置和 modules.yaml，保留注释。
Edit application configurations and modules.yaml without losing comments.

实例按文本块编辑：一个实例拥有它的 `- module:` 列表项和紧挨在上方的注释行，增加、删除和
改名时注释随实例移动。一个实例内的值经这个实例块的 YAML 往返修改。每次写入都使用
`xrobot format` 的规范写法：YAML 允许时 C++ 代码不加引号，否则加单引号；C++ 字符串用双
引号；集合保持原来的块格式或流格式。
Instances are edited as text blocks: an instance owns its `- module:` item and the comment
lines directly above it, so adding, removing or renaming an instance moves its comments
with it. Values inside one instance are edited through a YAML round trip of that instance
block. Every write uses the canonical layout that `xrobot format` enforces: C++ code
without quotes where YAML allows it and in single quotes otherwise, C++ strings in double
quotes, and collections in the block or flow style they were written in.
"""

from __future__ import annotations

import difflib
import hashlib
import io
import re
from pathlib import Path

import yaml
from ruamel.yaml import YAML
from ruamel.yaml.comments import CommentedMap, CommentedSeq
from ruamel.yaml.compat import ordereddict
from ruamel.yaml.nodes import ScalarNode
from ruamel.yaml.resolver import VersionedResolver
from ruamel.yaml.scalarstring import (
    DoubleQuotedScalarString,
    PlainScalarString,
    SingleQuotedScalarString,
)

from xrobot.config import (
    NULL_SCALARS,
    ConfigError,
    Located,
    cpp_string_literal,
    identifier_problem,
    parse_yaml,
    scalar_style,
    string_literal_content,
    validate_config,
)
from xrobot.constructor_model import (
    compliant_constructors,
    initializer_text,
    initializer_tree,
    qualify,
    replace_names,
    template_bindings,
)
from xrobot.module_parser import module_interface, select_module
from xrobot.project import atomic_write
from xrobot.source_syntax import code_tokens
from xrobot.type_index import ClassEntry, TypeIndex

# 顶层列表项的 "- " 在 ruamel 写出时所在的列（indent 的 offset）。
# The column of a top-level list item's "- " as ruamel writes it (the indent offset).
_SEQUENCE_OFFSET = 2
_BOM = "\ufeff"


class _TextResolver(VersionedResolver):
    """读取时不推断类型：null、~ 和空值表示未填写，其余标量都是文本，与 config.parse_yaml 一致。
    No implicit types: null, ~ and empty mean not filled in and every other scalar is
    text, as config.parse_yaml reads them.
    """

    def resolve(self, kind, value, implicit):
        """未加引号的标量一律解析为文本，null、~ 和空值解析为 null。
        Resolve every plain scalar as text, and null, ~ and empty values as null.
        """
        if kind is ScalarNode and implicit[0]:
            if value in NULL_SCALARS:
                return super().resolve(kind, "null", implicit)
            return self.DEFAULT_SCALAR_TAG
        return super().resolve(kind, value, implicit)


def _yaml() -> YAML:
    """配置文件用的 ruamel 往返读写器。
    The ruamel round-trip reader and writer for configurations.
    """
    document = YAML()
    document.Resolver = _TextResolver
    document.preserve_quotes = True
    document.width = 4096
    document.indent(mapping=2, sequence=4, offset=_SEQUENCE_OFFSET)
    return document


def _to_code(node):
    """把读入的值统一为 C++ 文本：双引号的值成为字符串字面量，其余原样。
    Turn loaded values into C++ text: a double-quoted value becomes a string literal,
    any other value stays as written.
    """
    # 直接赋值：ruamel 的容器赋值时会沿用旧值的引号类型。
    # Assign directly: ruamel's containers keep the quoting type of the old value.
    if isinstance(node, CommentedMap):
        for key in list(node):
            ordereddict.__setitem__(node, key, _to_code(node[key]))
    elif isinstance(node, CommentedSeq):
        for i, child in enumerate(node):
            list.__setitem__(node, i, _to_code(child))
    elif isinstance(node, DoubleQuotedScalarString):
        return cpp_string_literal(str(node))
    elif isinstance(node, str):
        return str(node)
    return node


def _restyle(node, flow: bool = False):
    """按规范写法给每个值定引号；集合保持原来的块格式或流格式。
    Give every value its canonical quoting; collections keep their block or flow style.

    Args:
        flow: 值位于流式集合（{...} 或 [...]）中。
            The value sits in a flow collection ({...} or [...]).
    """
    if isinstance(node, (CommentedMap, CommentedSeq)):
        flow = flow or bool(node.fa.flow_style())
    if isinstance(node, dict):
        setter = ordereddict.__setitem__ if isinstance(node, CommentedMap) else dict.__setitem__
        for key in list(node):
            setter(node, key, _restyle(node[key], flow))
        return node
    if isinstance(node, list):
        for i, child in enumerate(node):
            list.__setitem__(node, i, _restyle(child, flow))
        return node
    if not isinstance(node, str):
        return node
    code = str(node)
    style = scalar_style(code, flow)
    if style == '"':
        return DoubleQuotedScalarString(string_literal_content(code))
    if style is None:
        return PlainScalarString(code)
    return SingleQuotedScalarString(code)


def _load(text: str):
    """读取一段配置 YAML 以便往返编辑，值统一为 C++ 文本。
    Load configuration YAML for a round-trip edit, with every value as C++ text.
    """
    return _to_code(_yaml().load(text))


def dump_text(data) -> str:
    """按规范写法输出 YAML。
    Write YAML in the canonical layout.
    """
    stream = io.StringIO()
    _yaml().dump(_restyle(data), stream)
    return stream.getvalue()


def _normalized(text: str) -> str:
    """去掉 BOM，换行统一为 LF。
    Drop a BOM and turn line endings into LF.
    """
    if text.startswith(_BOM):
        text = text[1:]
    return text.replace("\r\n", "\n")


def canonical_text(text: str) -> str:
    """YAML 文档的规范写法，保留注释。
    The canonical layout of a YAML document, comments kept.
    """
    text = _normalized(text)
    data = _load(text)
    if data is None:
        return text if text.endswith("\n") or not text else text + "\n"
    return dump_text(data)


def file_hash(path: str | Path) -> str:
    """文件换行统一为 LF 后内容的 SHA-256，供 --if-match 比较。
    The SHA-256 of a file's content with LF line endings, compared by --if-match.
    """
    return hashlib.sha256(Path(path).read_bytes().replace(b"\r\n", b"\n")).hexdigest()


# -- instance blocks -------------------------------------------------------------


class Blocks:
    """一份配置中 modules 列表各项所占的行。
    The lines each item of the modules list occupies in one configuration.

    实例拥有紧挨在它上方的注释和空行；列表末尾的注释留在文件里。
    An instance owns the comment and blank lines directly above it; comments after the
    last item stay with the file.
    """

    def __init__(self, text: str) -> None:
        """找出 text 中 modules 列表每一项所占的行。
        Find the lines each item of the modules list occupies in text.
        """
        self.text = _normalized(text)
        self.lines = self.text.split("\n")
        root = yaml.compose(self.text, Loader=yaml.BaseLoader) if self.text.strip() else None
        self.items: list[list[int]] = []  # (comment_start, start, end) line indices, end exclusive
        self.sequence_end: int | None = None
        self.item_indent: int | None = None
        if root is None or not isinstance(root, yaml.MappingNode):
            return
        key = next((k for k, v in root.value if k.value == "modules"), None)
        node = next((v for k, v in root.value if k.value == "modules"), None)
        if node is None or not isinstance(node, yaml.SequenceNode) or node.flow_style:
            return
        starts = [child.start_mark.line for child in node.value]
        end = self._block_end(node)
        self.sequence_end = end
        for k, start in enumerate(starts):
            item_end = starts[k + 1] if k + 1 < len(starts) else end
            comment_start = start
            while (
                comment_start > 0
                and self._is_comment_or_blank(comment_start - 1)
                and (k == 0 or comment_start - 1 >= starts[k - 1])
            ):
                comment_start -= 1
            if k == 0:
                comment_start = max(comment_start, key.start_mark.line + 1)
            self.items.append([comment_start, start, item_end])
        # 一项之后的注释和空行属于下一项。
        # An item's trailing comment/blank lines belong to the next item.
        for k in range(len(self.items) - 1):
            self.items[k][2] = self.items[k + 1][0]
        if starts:
            line = self.lines[starts[0]]
            self.item_indent = len(line) - len(line.lstrip(" "))

    def _is_comment_or_blank(self, index: int) -> bool:
        """第 index 行是否为注释或空行。
        Whether line index is a comment or blank.
        """
        stripped = self.lines[index].strip()
        return not stripped or stripped.startswith("#")

    def _block_end(self, node: yaml.SequenceNode) -> int:
        """modules 列表结束的行，不含其后的注释和空行。
        The line where the modules list ends, excluding comments and blank lines after it.
        """
        end = node.end_mark.line
        while (
            end > node.start_mark.line
            and end - 1 < len(self.lines)
            and self._is_comment_or_blank(end - 1)
        ):
            end -= 1
        return end

    def item_text(self, k: int) -> str:
        """第 k 个实例的文本，不含上方的注释。
        The text of instance k, without the comments above it.
        """
        _, start, end = self.items[k]
        return "\n".join(self.lines[start:end])

    def replace(self, k: int, new_lines: list[str], keep_comments: bool = True) -> str:
        """把第 k 个实例换成 new_lines 后的文件文本。
        The file text with instance k replaced by new_lines.
        """
        comment_start, start, end = self.items[k]
        begin = start if keep_comments else comment_start
        lines = self.lines[:begin] + new_lines + self.lines[end:]
        return "\n".join(lines)

    def remove(self, k: int) -> str:
        """删除第 k 个实例及其上方注释后的文件文本。
        The file text without instance k and the comments above it.
        """
        comment_start, _, end = self.items[k]
        return "\n".join(self.lines[:comment_start] + self.lines[end:])

    def append(self, new_lines: list[str]) -> str:
        """在 modules 列表末尾加上 new_lines 后的文件文本。
        The file text with new_lines added at the end of the modules list.
        """
        end = self.sequence_end if self.sequence_end is not None else len(self.lines)
        return "\n".join(self.lines[:end] + new_lines + self.lines[end:])


def _render_item(item: CommentedMap, indent: int) -> list[str]:
    """把一个实例写成缩进 indent 个空格的列表项。
    Write one instance as a list item indented by indent spaces.
    """
    text = dump_text([item])
    lines = text.rstrip("\n").split("\n")
    base = len(lines[0]) - len(lines[0].lstrip(" "))
    return [" " * indent + line[base:] if line.strip() else "" for line in lines]


def _load_item(block_text: str) -> tuple[CommentedMap, int]:
    """读取一个实例块（只有一项的列表）以便往返编辑。
    Load one instance block (a one-item list) for a round-trip edit.

    块按 ruamel 写出顶层列表项的位置读入（"- " 在第 _SEQUENCE_OFFSET 列），注释的列号与
    写回时的键对齐，往返后注释不会移位。
    The block is loaded where ruamel writes a top-level list item ("- " at column
    _SEQUENCE_OFFSET), so comment columns line up with the keys as written back and a
    round trip does not shift the comments.

    Returns:
        (实例映射, 原来的缩进)。
        (instance mapping, original indentation).

    Raises:
        ConfigError: 块不是单个列表项。
            The block is not a single list item.
    """
    lines = block_text.split("\n")
    indent = min(len(line) - len(line.lstrip(" ")) for line in lines if line.strip())
    shifted = [" " * _SEQUENCE_OFFSET + line[indent:] if line.strip() else "" for line in lines]
    data = _load("\n".join(shifted))
    if not isinstance(data, CommentedSeq) or len(data) != 1:
        raise ConfigError("cannot edit this instance: its YAML block is not a single list item")
    return data[0], indent


class ConfigFile:
    """一份应用配置：文本和解析结果；不存在的文件视为空配置。
    One application configuration: its text and parsed values; a missing file reads as an
    empty configuration.
    """

    def __init__(self, path: str | Path, source: str | None = None) -> None:
        """读取 path；source 是报错中文件的名字，缺省为 path。
        Read path; source is the file's name in errors, path by default.
        """
        self.path = Path(path)
        self.source = source or self.path.as_posix()
        self.text = (
            self.path.read_text(encoding="utf-8-sig") if self.path.exists() else "modules: []\n"
        )
        self.config = parse_yaml(self.text, self.source)

    def blocks(self) -> Blocks:
        """当前文本的实例块。
        The instance blocks of the current text.
        """
        return Blocks(self.text)

    def index_of(self, instance_id: str) -> int:
        """实例在 modules 列表中的位置。
        The position of an instance in the modules list.

        Raises:
            ConfigError: 没有这个实例。
                There is no such instance.
        """
        for k, entry in enumerate(self.config.get("modules") or []):
            if isinstance(entry, dict) and entry.get("id") == instance_id:
                return k
        raise ConfigError(f"{self.source}: no instance with id {instance_id}")

    def write(self, text: str, check: bool = True) -> None:
        """按规范写法写入 text。
        Write text in the canonical layout.

        Args:
            check: 写入前检查配置结构；添加实例和同步时不检查，文件中其他地方的问题不妨碍
                这两个操作。
                Check the configuration structure first; adding an instance and syncing
                skip it, so problems elsewhere in the file do not block them.

        Raises:
            ConfigError: check 时结构有错；文件不变。
                With check, the structure has errors; the file is unchanged.
        """
        text = canonical_text(text)
        config = parse_yaml(text, self.source)
        if check:
            validate_config(config, self.source)
        atomic_write(self.path, text)
        self.text, self.config = text, config


# -- instance set / remove / rename ---------------------------------------------

_PATH_HINT = "use template_args[n] or args.<param>[.<field>|[n]]..."


def _path_tokens(path: str) -> list[str]:
    """把值路径拆成名字和 [n]，只接受 args 和 template_args 下的路径。
    Split a value path into names and [n]; only paths under args and template_args are
    accepted.

    Raises:
        ConfigError: 路径写法不对，或指向 id、module。
            The path is malformed, or points at id or module.
    """
    tokens = re.findall(r"[A-Za-z_][A-Za-z_0-9]*|\[\d+\]", path)
    if not tokens or "".join(t if t.startswith("[") else "." + t for t in tokens)[1:] != path:
        raise ConfigError(f"invalid path {path}; {_PATH_HINT}")
    if tokens[0] == "id":
        raise ConfigError(
            "an instance id is changed with `xrobot instance rename`, which also updates the "
            "references to it"
        )
    if tokens[0] == "module":
        raise ConfigError(
            "the Module of an instance cannot be changed; remove the instance and add the "
            "other Module"
        )
    if tokens[0] not in ("args", "template_args"):
        raise ConfigError(f"invalid path {path}; {_PATH_HINT}")
    return tokens


def _set_path(item: CommentedMap, path: str, value) -> None:
    """在一个实例映射中按路径设置值。
    Set a value at a path inside one instance mapping.

    args.<参数名> 按名字找到 args 列表中的那一项；args 本身可整体替换为一个列表（换用另一个
    构造函数时）。
    args.<param> finds that item of the args list by name; args itself can be replaced by
    a whole list (when switching to another constructor).

    Raises:
        ConfigError: 路径不存在，或 args 的新值不是单参数映射的列表。
            The path does not exist, or a new args value is not a list of one-parameter
            mappings.
    """
    tokens = _path_tokens(path)
    node = item
    if tokens[0] == "args":
        if len(tokens) == 1:
            if not isinstance(value, list) or not all(
                isinstance(v, dict) and len(v) == 1 for v in value
            ):
                raise ConfigError(
                    "args takes a list of one-parameter mappings, e.g. "
                    '[{"led": "LED_B"}, {"cycle": "250"}]'
                )
            item["args"] = value
            return
        name = tokens[1]
        node = next((e for e in item.get("args") or [] if isinstance(e, dict) and name in e), None)
        if node is None:
            raise ConfigError(f"path {path}: no argument {name}")
        tokens = tokens[1:]
    for position, token in enumerate(tokens):
        if token.startswith("["):
            key = int(token[1:-1])
            if not isinstance(node, list) or key >= len(node):
                raise ConfigError(f"path {path}: no element {key}")
        else:
            key = token
            if not isinstance(node, dict) or key not in node:
                raise ConfigError(f"path {path}: no key {key}")
        if position == len(tokens) - 1:
            node[key] = value
            return
        node = node[key]


def _to_yaml_value(value):
    """命令行给出的值转为配置值：数字和布尔值成为 C++ 文本，映射和列表逐项转换。
    Turn a value given on the command line into a configuration value: numbers and
    booleans become C++ text, mappings and lists are converted item by item.
    """
    if isinstance(value, bool):
        return "true" if value else "false"
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


def set_value(
    config_path: str | Path,
    instance_id: str,
    path: str,
    value,
    if_match: str | None = None,
    source: str | None = None,
) -> None:
    """替换一个实例中的一个值；其他实例的文本不变。
    Replace one value of one instance; the text of the other instances is unchanged.

    Args:
        if_match: 读取文件时的 file_hash；文件此后被改过则拒绝写入。
            The file_hash when the file was read; the write is refused if the file changed
            since.

    Raises:
        ConfigError: 文件已被改过、没有这个实例、路径不对，或结果不是有效配置。
            The file changed, the instance or path does not exist, or the result is not a
            valid configuration.
    """
    config = ConfigFile(config_path, source)
    if if_match is not None and file_hash(config.path) != if_match:
        raise ConfigError(f"{config.source} changed since it was read; reload and retry")
    k = config.index_of(instance_id)
    blocks = config.blocks()
    item, indent = _load_item(blocks.item_text(k))
    _set_path(item, path, _to_yaml_value(value))
    config.write(blocks.replace(k, _render_item(item, indent)))


def remove_instance(config_path: str | Path, instance_id: str, source: str | None = None) -> None:
    """删除一个实例及其上方的注释。
    Remove an instance and the comments above it.

    Raises:
        ConfigError: 没有这个实例，或它仍被其他实例引用。
            There is no such instance, or other instances still refer to it.
    """
    config = ConfigFile(config_path, source)
    k = config.index_of(instance_id)
    users = references_to(config.config, instance_id)
    if users:
        raise ConfigError(
            f"{config.source}: {instance_id} is still used by {', '.join(users)}; change those "
            "values first"
        )
    text = config.blocks().remove(k)
    if parse_yaml(text, config.source).get("modules") is None:
        text = re.sub(r"^modules:[ \t]*$", "modules: []", text, count=1, flags=re.M)
    config.write(text)


def _values(node):
    """一个值树中的全部标量文本。
    Every scalar text in a value tree.
    """
    if isinstance(node, dict):
        for child in node.values():
            yield from _values(child)
    elif isinstance(node, list):
        for child in node:
            yield from _values(child)
    elif isinstance(node, str):
        yield node


def _mentions(text: str, name: str) -> bool:
    """C++ 文本是否以未限定的名字用到 name（成员访问和限定名不算）。
    Whether C++ text uses name unqualified (member access and qualified names do not
    count).
    """
    items = code_tokens(text)
    for i, token in enumerate(items):
        if (
            token.kind == "identifier"
            and token.text == name
            and (not i or items[i - 1].text not in (".", "->", "::"))
        ):
            return True
    return False


def references_to(config: Located, instance_id: str) -> list[str]:
    """在 args 或 template_args 中用到 instance_id 的其他实例。
    The other instances whose args or template_args use instance_id.
    """
    users = []
    for entry in config.get("modules") or []:
        if not isinstance(entry, dict) or entry.get("id") == instance_id:
            continue
        values = list(_values(entry.get("args") or [])) + list(
            _values(entry.get("template_args") or [])
        )
        if any(_mentions(v, instance_id) for v in values):
            users.append(entry.get("id"))
    return users


def rename_instance(
    config_path: str | Path, instance_id: str, new_id: str, source: str | None = None
) -> None:
    """给实例改名，并替换同一配置中对它的全部引用；字符串内容不变。
    Rename an instance and every reference to it in the same configuration; string
    contents are left alone.

    Raises:
        ConfigError: 新 id 不合法或已存在，或没有这个实例。
            The new id is invalid or taken, or there is no such instance.
    """
    problem = identifier_problem(new_id)
    if problem:
        raise ConfigError(f"{new_id} {problem}")
    config = ConfigFile(config_path, source)
    if any(
        isinstance(e, dict) and e.get("id") == new_id for e in config.config.get("modules") or []
    ):
        raise ConfigError(f"{config.source}: instance id {new_id} already exists")
    target = config.index_of(instance_id)
    text = config.text
    blocks = Blocks(text)
    for k in reversed(range(len(blocks.items))):
        item, indent = _load_item(blocks.item_text(k))
        changed = False
        if k == target:
            item["id"] = new_id
            changed = True
        for key in ("args", "template_args"):
            if key in item and _rename_values(item[key], instance_id, new_id):
                changed = True
        if changed:
            text = blocks.replace(k, _render_item(item, indent))
            blocks = Blocks(text)
    config.write(text)


def _rename_values(node, old: str, new: str) -> bool:
    """把值树中对 old 的引用换成 new；有改动时返回 True。
    Replace the references to old with new in a value tree; True when anything changed.
    """
    changed = False
    if isinstance(node, dict):
        slots = list(node)
    elif isinstance(node, list):
        slots = range(len(node))
    else:
        return False
    for slot in slots:
        child = node[slot]
        if isinstance(child, str):
            if _mentions(child, old):
                node[slot] = replace_names(str(child), {old: new})
                changed = True
        elif _rename_values(child, old, new):
            changed = True
    return changed


# -- seeding (instance add, sync, Module compile probe) --------------------------


def _field_default(entry: ClassEntry, name: str, index: TypeIndex, spelled: str) -> str | None:
    """字段的默认成员初始化，名字已限定；没有时为 None。
    A field's default member initializer with names qualified; None when it has none.
    """
    text = entry.layout().field_defaults.get(name)
    return index.qualify_in(text, entry, spelled) if text is not None else None


def seed_value(default, target: str | None, index: TypeIndex | None, scope: tuple = ()):
    """由源码中的默认值得到配置值。
    The configuration value for a default taken from the source.

    默认值是 {} 或指定初始化器、类型是索引中的聚合体时，写成完整映射：按顺序列出全部字段，
    每个字段取给出的值或默认成员初始化，都没有时为 {}。类型不在索引中时，按位置的初始化器
    写成 C++ 代码，其他默认值（工厂函数、常量、转换）原样保留。
    For a {} or designated default of an aggregate in the index, the value is a complete
    mapping: every field in order, each with the given value or its default member
    initializer, else {}. For a type outside the index, a positional initializer is written
    as C++ code; any other default (factory calls, constants, casts) is kept as written.
    """
    if default is None:
        return None
    tree = initializer_tree(default, target) if isinstance(default, str) else default
    entry = index.resolve(target, scope) if index is not None and target else None
    if (
        entry is not None
        and entry.is_aggregate()
        and entry.mapping_problem() is None
        and (tree == [] or isinstance(tree, dict))
    ):
        spelled = re.sub(r"^\s*(const\s+)?", "", target).rstrip("&* ").replace(" const", "")
        result = CommentedMap()
        given = tree if isinstance(tree, dict) else {}
        for name, cpp_type, _ in entry.fields():
            field_type = index.qualify_in(cpp_type, entry, spelled)
            value = given[name] if name in given else _field_default(entry, name, index, spelled)
            if isinstance(value, (dict, list)) or value is None or isinstance(value, str):
                child = seed_value(
                    value if value is not None else "{}", field_type, index, entry.path
                )
            else:
                child = value
            result[name] = child if child is not None else "{}"
        return result
    if isinstance(tree, dict):
        result = CommentedMap()
        for name, value in tree.items():
            result[name] = seed_value(value, None, None) if not isinstance(value, str) else value
        return result
    if isinstance(tree, list):
        # 按位置的初始化器按 C++ 原文写入。
        # A positional initializer is written as C++ code.
        return default if isinstance(default, str) else initializer_text(tree)
    return default


def seed_arguments(
    interface: dict, cpp_class: str, templates: dict[str, str], index: TypeIndex
) -> list[dict]:
    """第一个符合约定的构造函数的参数，值取源码默认值；依赖参数为 None（未填写）。
    The arguments of the first compliant constructor with their source defaults;
    dependencies are None (not filled in).
    """
    ctor = compliant_constructors(interface)[0]
    result = []
    for p in ctor["arguments"]:
        if p["default"] is None:
            value = None
        else:
            default = qualify(p["default"], interface, cpp_class, templates)
            target = qualify(p["type"], interface, cpp_class, templates, True)
            value = seed_value(default, target, index)
        result.append({p["name"]: value})
    return result


def instance_item(
    module_id: str,
    identity: str,
    cpp_class: str,
    interface: dict,
    index: TypeIndex | None,
    template_values: list[str] | None = None,
) -> CommentedMap:
    """`instance add` 写入的实例：模板实参和参数取源码默认值，依赖参数留空。
    The instance `instance add` writes: template arguments and arguments take their source
    defaults, dependencies are left unfilled.

    有模板参数没有值时只写出 template_args，参数要等模板实参填好后由 sync 补上。
    When a template parameter has no value only template_args is written; sync adds the
    arguments once the template arguments are filled in.

    Args:
        template_values: 按位置给出的模板实参；其余取默认值。
            Template arguments given by position; the rest take their defaults.
    """
    item = CommentedMap()
    item["module"] = module_id
    item["id"] = identity
    given = list(template_values or [])
    template_values = given + [p["default"] for p in interface["template_parameters"][len(given) :]]
    if template_values:
        item["template_args"] = CommentedSeq(template_values)
    if all(v is not None for v in template_values):
        templates = template_bindings(interface, template_values)
        spelled = cpp_class + ("<" + ", ".join(template_values) + ">" if template_values else "")
        arguments = seed_arguments(interface, spelled, templates, index)
        if arguments:
            args = CommentedSeq()
            for argument in arguments:
                mapping = CommentedMap()
                for key, value in argument.items():
                    mapping[key] = value
                args.append(mapping)
            item["args"] = args
    return item


def instance_text(item: CommentedMap) -> str:
    """只有这一个实例的配置文本，与 `instance add` 写入空配置的结果相同。
    The configuration text holding only this instance, as `instance add` writes it into an
    empty configuration.
    """
    return "modules:\n" + "\n".join(_render_item(item, _SEQUENCE_OFFSET)) + "\n"


def next_instance_id(modules: list, base_name: str) -> str:
    """未被占用的实例 id：<类名小写>_<n>。
    An unused instance id: <lower-case class name>_<n>.
    """
    prefix = base_name.lower() + "_"
    names = {entry.get("id", "") for entry in modules if isinstance(entry, dict)}
    number = 0
    while prefix + str(number) in names:
        number += 1
    return prefix + str(number)


def add_instance(
    config_path: str | Path,
    module_name: str,
    modules: dict,
    index: TypeIndex,
    instance_id: str | None = None,
    source: str | None = None,
) -> str:
    """在配置末尾添加一个实例，参数取源码默认值，依赖参数留空。
    Add an instance at the end of the configuration, with source defaults and the
    dependencies left unfilled.

    Returns:
        新实例的 id。
        The id of the new instance.

    Raises:
        ConfigError: 模块是库，或 id 不合法。
            The Module is a library, or the id is invalid.
    """
    config = ConfigFile(config_path, source)
    module = select_module(modules, module_name)
    if not module["manifest"].standalone:
        raise ConfigError(
            f"{module['id']} is a library (standalone: false) and cannot be instantiated"
        )
    interface = module_interface(module)
    identity = instance_id or next_instance_id(config.config.get("modules") or [], module["name"])
    problem = identifier_problem(identity)
    if problem:
        raise ConfigError(f"instance id {identity} {problem}")
    item = instance_item(module["id"], identity, module["name"], interface, index)
    blocks = config.blocks()
    indent = blocks.item_indent if blocks.item_indent is not None else 2
    if blocks.sequence_end is None:
        text = config.text.rstrip("\n")
        text = re.sub(r"^modules:\s*\[\]\s*$", "modules:", text, flags=re.M)
        if not re.search(r"^modules:", text, flags=re.M):
            text = "modules:\n" + text
        lines = text.split("\n")
        at = next(i for i, line in enumerate(lines) if line.startswith("modules:")) + 1
        new_text = "\n".join(lines[:at] + _render_item(item, indent) + lines[at:]) + "\n"
    else:
        new_text = blocks.append(_render_item(item, indent))
    config.write(new_text, check=False)
    return identity


# -- sync ------------------------------------------------------------------------


def sync_config(
    config_path: str | Path, modules: dict, index: TypeIndex, source: str | None = None
) -> str:
    """按模块当前的构造函数和字段更新配置：补上新字段和带默认值的新参数，去掉已删除的字段。
    Bring a configuration up to the Modules' current constructors and fields: add new
    fields and defaulted parameters, drop removed fields.

    Returns:
        改动的 unified diff；没有改动时为空字符串。
        A unified diff of the changes; empty when nothing changed.
    """
    config = ConfigFile(config_path, source)
    before = config.text
    text = before
    blocks = Blocks(text)
    entries = config.config.get("modules") or []
    for k in reversed(range(len(blocks.items))):
        entry = entries[k] if k < len(entries) else None
        if not isinstance(entry, dict) or not entry.get("module"):
            continue
        try:
            module = select_module(modules, entry["module"])
            interface = module_interface(module)
        except ValueError:
            continue
        template_args = [str(v) for v in entry.get("template_args") or [] if v is not None]
        if len(template_args) != len(entry.get("template_args") or []):
            continue
        cpp_class = module["name"] + (
            "<" + ", ".join(template_args) + ">" if interface["template_parameters"] else ""
        )
        try:
            templates = template_bindings(interface, template_args)
        except ValueError:
            continue
        item, indent = _load_item(blocks.item_text(k))
        if _sync_item(item, interface, cpp_class, templates, index):
            text = blocks.replace(k, _render_item(item, indent))
            blocks = Blocks(text)
    if text != before:
        config.write(text, check=False)
    return "".join(
        difflib.unified_diff(
            before.splitlines(True),
            canonical_text(text).splitlines(True),
            config.source,
            config.source,
        )
    )


def _sync_item(
    item: CommentedMap,
    interface: dict,
    cpp_class: str,
    templates: dict[str, str],
    index: TypeIndex,
) -> bool:
    """同步一个实例；有改动时返回 True。
    Sync one instance; True when anything changed.

    参数名与任何构造函数都不完全一致时，只有恰好一个构造函数以这些参数名开头、其余参数都有
    默认值，才在末尾补上其余参数。
    When the parameter names match no constructor exactly, the remaining parameters are
    appended only if exactly one constructor starts with these names and gives every
    remaining parameter a default.
    """
    args = item.get("args")
    names = [next(iter(a)) for a in args or [] if isinstance(a, dict) and a]
    ctors = compliant_constructors(interface)
    exact = [c for c in ctors if [p["name"] for p in c["arguments"]] == names]
    changed = False
    if not exact:
        extended = [
            c
            for c in ctors
            if [p["name"] for p in c["arguments"]][: len(names)] == names
            and all(p["default"] is not None for p in c["arguments"][len(names) :])
        ]
        if len(extended) != 1:
            return False
        ctor = extended[0]
        if args is None:
            args = CommentedSeq()
            item["args"] = args
        for p in ctor["arguments"][len(names) :]:
            mapping = CommentedMap()
            default = qualify(p["default"], interface, cpp_class, templates)
            target = qualify(p["type"], interface, cpp_class, templates, True)
            mapping[p["name"]] = seed_value(default, target, index)
            args.append(mapping)
        changed = True
    else:
        ctor = exact[0]
    for p, argument in zip(ctor["arguments"], args or [], strict=False):
        value = argument.get(p["name"])
        if isinstance(value, dict):
            target = qualify(p["type"], interface, cpp_class, templates, True)
            defaults = None
            if p["default"] is not None:
                defaults = initializer_tree(
                    qualify(p["default"], interface, cpp_class, templates), target
                )
            if _sync_mapping(value, target, index, (), defaults):
                changed = True
    return changed


def _sync_mapping(
    value: CommentedMap,
    target: str | None,
    index: TypeIndex,
    scope: tuple,
    defaults=None,
) -> bool:
    """让映射与它的结构体一致；新字段取参数默认值中给出的值，否则取字段的默认成员初始化。
    Match a mapping to its struct; a new field takes the value the parameter's designated
    default gives it, else the field's default member initializer.
    """
    entry = index.resolve(target, scope) if target else None
    if entry is None or entry.mapping_problem() is not None:
        return False
    spelled = re.sub(r"^\s*(const\s+)?", "", target).rstrip("&* ")
    if not entry.is_aggregate():
        return _sync_constructor_mapping(value, entry, index, spelled)
    fields = entry.fields()
    wanted = [name for name, _, _ in fields]
    changed = False
    for key in list(value):
        if key not in wanted:
            del value[key]
            changed = True
    for position, (name, cpp_type, _) in enumerate(fields):
        field_type = index.qualify_in(cpp_type, entry, spelled)
        given = defaults.get(name) if isinstance(defaults, dict) else None
        if name not in value:
            default = given if given is not None else _field_default(entry, name, index, spelled)
            value.insert(
                position,
                name,
                seed_value(default if default is not None else "{}", field_type, index, entry.path)
                or "{}",
            )
            changed = True
        elif isinstance(value[name], dict):
            if _sync_mapping(value[name], field_type, index, entry.path, given):
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


def _sync_constructor_mapping(
    value: CommentedMap, entry: ClassEntry, index: TypeIndex, spelled: str
) -> bool:
    """按构造参数写的映射：与任何构造函数都不一致且类只有一个构造函数时，保留同名的值、
    补上新参数的默认值、去掉已删除的参数。
    A mapping keyed by constructor parameters: when it matches no constructor and the class
    has exactly one, keep same-named values, add new parameters with their defaults and
    drop removed ones.
    """
    constructors = [c for c in entry.constructors() if c]
    keys = list(value)
    if any([p["name"] for p in c] == keys for c in constructors) or len(constructors) != 1:
        return False
    parameters = constructors[0]
    if any(p["default"] is None and p["name"] not in value for p in parameters):
        return False  # a new parameter without a default cannot be filled in
    items = []
    for p in parameters:
        if p["name"] in value:
            items.append((p["name"], value[p["name"]]))
        else:
            items.append((p["name"], index.qualify_in(p["default"], entry, spelled)))
    for key in list(value):
        del value[key]
    for key, child in items:
        value[key] = child
    return True


# -- modules.yaml ----------------------------------------------------------------


def _request_lines(text: str):
    """modules.yaml 的各行、modules 键、每个块列表项的 (行号, 节点)，以及列表节点。
    The lines of modules.yaml, the modules key, the (line, node) of each block-list item
    and the list node.

    Raises:
        ConfigError: modules 不是列表。
            modules is not a list.
    """
    lines = text.split("\n")
    root = yaml.compose(text, Loader=yaml.BaseLoader) if text.strip() else None
    if root is None:
        return lines, None, [], None
    key = next((k for k, v in root.value if k.value == "modules"), None)
    node = next((v for k, v in root.value if k.value == "modules"), None)
    if node is None or not isinstance(node, yaml.SequenceNode):
        raise ConfigError("modules.yaml: modules must be a list")
    items = [(child.start_mark.line, child) for child in node.value]
    return lines, key, items, node


def add_module(modules_yaml: str | Path, request_text: str) -> None:
    """在 modules.yaml 末尾加一行请求，其余内容不变；没写 ref 时为 @same-or-dev。
    Append one request line to modules.yaml, leaving the rest unchanged; a request
    without a ref gets @same-or-dev.

    Raises:
        ConfigError: 已经请求过这个模块，或 modules 是非空的流式列表。
            The Module is already requested, or modules is a non-empty flow list.
    """
    from xrobot.lock import request

    parsed = request(request_text, canonical=True)
    path = Path(modules_yaml)
    text = (
        path.read_text(encoding="utf-8-sig").replace("\r\n", "\n")
        if path.exists()
        else "modules: []\n"
    )
    lines, key, items, node = _request_lines(text)
    for _, child in items:
        if isinstance(child, yaml.ScalarNode):
            existing = request(child.value)
        else:
            existing = request({k.value: v.value for k, v in child.value})
        if existing["id"].casefold() == parsed["id"].casefold():
            raise ConfigError(f"{parsed['id']} is already requested in {modules_yaml}")
    value = request_text if "@" in request_text else request_text + "@same-or-dev"
    if key is None:
        lines = [line for line in lines if line.strip()] + ["modules:", "  - " + value]
    elif node.flow_style:
        if items:
            raise ConfigError(
                f"{modules_yaml} uses a flow list for modules; write it as a block list first"
            )
        line = lines[key.start_mark.line]
        lines[key.start_mark.line] = line[: key.start_mark.column] + "modules:"
        lines.insert(key.start_mark.line + 1, "  - " + value)
    else:
        last_line, last = items[-1]
        prefix = lines[last_line][: last.start_mark.column]
        end = last.end_mark.line + (1 if last.end_mark.column else 0)
        lines.insert(max(end, last_line + 1), prefix + value)
    atomic_write(path, "\n".join(lines).rstrip("\n") + "\n")


def remove_module(modules_yaml: str | Path, identity: str) -> None:
    """从 modules.yaml 删除一行请求，其余内容不变。
    Delete one request line from modules.yaml, leaving the rest unchanged.

    Raises:
        ConfigError: 没有请求这个模块，或 modules 是流式列表。
            The Module is not requested, or modules is a flow list.
    """
    from xrobot.lock import request

    path = Path(modules_yaml)
    text = path.read_text(encoding="utf-8-sig").replace("\r\n", "\n")
    lines, key, items, node = _request_lines(text)
    for index, (line, child) in enumerate(items):
        if isinstance(child, yaml.ScalarNode):
            existing = request(child.value)
        else:
            existing = request({k.value: v.value for k, v in child.value})
        if existing["id"].casefold() != identity.casefold():
            continue
        if node.flow_style:
            raise ConfigError(
                f"{modules_yaml} uses a flow list for modules; write it as a block list first"
            )
        end = (
            items[index + 1][0]
            if index + 1 < len(items)
            else child.end_mark.line + (1 if child.end_mark.column else 0)
        )
        del lines[line : max(end, line + 1)]
        if len(items) == 1:
            lines[key.start_mark.line] = lines[key.start_mark.line].rstrip() + " []"
        atomic_write(path, "\n".join(lines).rstrip("\n") + "\n")
        return
    raise ConfigError(f"{identity} is not requested in {modules_yaml}")


def format_files(paths: list[Path], check: bool = False) -> list[Path]:
    """把文件改写为规范写法；check 时只报告，不写。
    Rewrite files in the canonical layout; with check only report them.

    Returns:
        不是规范写法的文件。
        The files that are not in the canonical layout.
    """
    changed = []
    for path in paths:
        path = Path(path)
        original = path.read_bytes().decode("utf-8")
        formatted = canonical_text(original)
        if formatted != original:
            changed.append(path)
            if not check:
                atomic_write(path, formatted)
    return changed
