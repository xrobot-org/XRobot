"""读取和检查 XRobot 应用配置。
Load and validate XRobot application configurations.

配置是普通 YAML。不加引号或用单引号的值是 C++ 代码，双引号的值是 C++ 字符串；读取时记下每个
节点的行号，生成时的报错和头文件中的 #line 指令都能指回 YAML。
A configuration is plain YAML. A value without quotes or in single quotes is C++ code, and a
double-quoted value is a C++ string. Loading keeps the line of every node, so generation
errors and the #line directives of the generated header point back into the YAML.
"""

import re
from pathlib import Path

import yaml

from xrobot.source_syntax import code_tokens

NULL_SCALARS = ("", "~", "null", "Null", "NULL")
TOP_LEVEL = ("modules", "settings", "constexprs", "constexpr_namespace", "constexpr_includes")
IDENTIFIER = r"[A-Za-z_][A-Za-z_0-9]*"

# C++20 keywords and alternative tokens; an instance id or registration name
# spelled like one of these cannot become a C++ object name.
CPP_KEYWORDS = frozenset(
    [
        "alignas",
        "alignof",
        "and",
        "and_eq",
        "asm",
        "auto",
        "bitand",
        "bitor",
        "bool",
        "break",
        "case",
        "catch",
        "char",
        "char8_t",
        "char16_t",
        "char32_t",
        "class",
        "compl",
        "concept",
        "const",
        "consteval",
        "constexpr",
        "constinit",
        "const_cast",
        "continue",
        "co_await",
        "co_return",
        "co_yield",
        "decltype",
        "default",
        "delete",
        "do",
        "double",
        "dynamic_cast",
        "else",
        "enum",
        "explicit",
        "export",
        "extern",
        "false",
        "float",
        "for",
        "friend",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "mutable",
        "namespace",
        "new",
        "noexcept",
        "not",
        "not_eq",
        "nullptr",
        "operator",
        "or",
        "or_eq",
        "private",
        "protected",
        "public",
        "register",
        "reinterpret_cast",
        "requires",
        "return",
        "short",
        "signed",
        "sizeof",
        "static",
        "static_assert",
        "static_cast",
        "struct",
        "switch",
        "template",
        "this",
        "thread_local",
        "throw",
        "true",
        "try",
        "typedef",
        "typeid",
        "typename",
        "union",
        "unsigned",
        "using",
        "virtual",
        "void",
        "volatile",
        "wchar_t",
        "while",
        "xor",
        "xor_eq",
        "final",
        "override",
        "import",
        "module",
    ]
)

# Macros that LibXR, the C library or the generated header define; an object
# with one of these names does not survive preprocessing.
RESERVED_MACROS = frozenset(
    [
        "ASSERT",
        "ASSERT_FROM_CALLBACK",
        "UNUSED",
        "NULL",
        "EOF",
        "assert",
        "errno",
        "offsetof",
        "XR_REGISTER",
        "XROBOT_MAIN",
        "XR_LOG_DEBUG",
        "XR_LOG_INFO",
        "XR_LOG_PASS",
        "XR_LOG_WARN",
        "XR_LOG_ERROR",
        "M_PI",
        "M_2PI",
        "M_1G",
    ]
)

RESERVED_NAMESPACES = frozenset(("std", "LibXR", "xrobot_generated"))

# 只含这些转义的 C++ 字符串字面量才能写成 YAML 双引号值，并能原样读回。
# A C++ string literal with only these escapes can be written as a YAML double-quoted
# value and read back unchanged.
_ESCAPES = {"\\": "\\\\", '"': '\\"', "\n": "\\n", "\t": "\\t", "\r": "\\r"}
_UNESCAPES = {"\\": "\\", '"': '"', "n": "\n", "t": "\t", "r": "\r"}
_SIMPLE_LITERAL = re.compile(r'"((?:[^"\\\x00-\x1f\x7f]|\\[\\"ntr])*)"')


class ConfigError(ValueError):
    """带有 <配置>: <位置> 前缀的配置错误。
    A configuration error that already carries its <config>: <path> prefix.
    """


class Located(dict):
    """记录每个键所在 YAML 行号的配置映射。
    A configuration mapping that also records the YAML line of each key.
    """

    def __init__(self, line: int) -> None:
        """建立空映射，记下它在 YAML 中所在的行。
        Create an empty mapping located at line of the YAML.
        """
        super().__init__()
        self.line = line
        self.key_lines: dict[str, int] = {}


class LocatedList(list):
    """记录每一项所在 YAML 行号的配置列表。
    A configuration list that also records the YAML line of each item.
    """

    def __init__(self, line: int) -> None:
        """建立空列表，记下它在 YAML 中所在的行。
        Create an empty list located at line of the YAML.
        """
        super().__init__()
        self.line = line
        self.item_lines: list[int] = []


def cpp_string_literal(content: str) -> str:
    """把一段文字写成 C++ 字符串字面量。
    Write a piece of text as a C++ string literal.
    """
    parts = []
    for char in content:
        if char in _ESCAPES:
            parts.append(_ESCAPES[char])
        elif ord(char) < 0x20 or ord(char) == 0x7F:
            parts.append(f"\\{ord(char):03o}")
        else:
            parts.append(char)
    return '"' + "".join(parts) + '"'


def string_literal_content(text: str) -> str | None:
    """text 恰好是一个只含简单转义的 C++ 字符串字面量时，返回它表示的文字，否则为 None。
    The text a C++ string literal stands for when text is exactly one such literal with
    only simple escapes; None otherwise.
    """
    match = _SIMPLE_LITERAL.fullmatch(text)
    if match is None:
        return None
    return re.sub(r"\\(.)", lambda m: _UNESCAPES[m.group(1)], match.group(1))


def _reads_back_plain(text: str, flow: bool) -> bool:
    """text 不加引号写在 YAML 中能否原样读回。
    Whether text written without quotes reads back unchanged from YAML.
    """
    try:
        node = yaml.compose("{k: " + text + "}" if flow else "k: " + text, Loader=yaml.BaseLoader)
    except yaml.YAMLError:
        return False
    child = node.value[0][1]
    return isinstance(child, yaml.ScalarNode) and child.style is None and child.value == text


def scalar_style(text: str, flow: bool = False) -> str | None:
    """规范写法中一个值的引号：'"' 为字符串，None 为不加引号的代码，"'" 为需要引号的代码。
    The quoting of a value in the canonical layout: '"' for a string, None for code
    without quotes, "'" for code that YAML only accepts in quotes.

    Args:
        text: 值的 C++ 文本。
            The C++ text of the value.
        flow: 值位于 YAML 流式集合（{...} 或 [...]）中。
            The value sits in a YAML flow collection ({...} or [...]).
    """
    if string_literal_content(text) is not None:
        return '"'
    if text not in NULL_SCALARS and "\n" not in text and _reads_back_plain(text, flow):
        return None
    return "'"


def _reject_anchors_and_tags(text: str, source: str) -> None:
    """拒绝 YAML 锚点、别名和标签。
    Reject YAML anchors, aliases and tags.

    Raises:
        ConfigError: 使用了其中之一；报错带行号。
            One is used; the message names the line.
    """
    for event in yaml.parse(text, Loader=yaml.BaseLoader):
        line = event.start_mark.line + 1
        if isinstance(event, yaml.AliasEvent) or getattr(event, "anchor", None):
            raise ConfigError(
                f"{source}:{line}: YAML anchors and aliases are not allowed; reference "
                "instances by id and share values through constexprs"
            )
        tag = getattr(event, "tag", None)
        if tag not in (None, "!"):
            raise ConfigError(
                f"{source}:{line}: YAML tags ({tag}) are not allowed; write the value as C++ text"
            )


def _construct(node: yaml.Node, source: str) -> "Located | LocatedList | str | None":
    """把 YAML 节点转成配置值：双引号的值成为 C++ 字符串字面量，其余标量原样作为 C++ 代码。
    Turn a YAML node into a configuration value: a double-quoted value becomes a C++
    string literal, any other scalar is C++ code as written.

    Raises:
        ConfigError: 键不是普通名字，或有重复的键。
            A key is not a plain name, or a key is repeated.
    """
    line = node.start_mark.line + 1
    if isinstance(node, yaml.ScalarNode):
        if node.style == '"':
            return cpp_string_literal(node.value)
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
            raise ConfigError(f"{source}:{key_line}: mapping keys must be plain names")
        key = key_node.value
        if key in result:
            raise ConfigError(f"{source}:{key_line}: duplicate key {key}")
        result.key_lines[key] = key_line
        result[key] = _construct(value_node, source)
    return result


def parse_yaml(text: str, source: str) -> "Located | LocatedList | str | None":
    """把配置 YAML 解析为带行号的值。
    Parse configuration YAML into values that carry their line numbers.

    Raises:
        ConfigError: YAML 语法错误，或使用了不允许的写法。
            A YAML syntax error, or a construct that is not allowed.
    """
    try:
        _reject_anchors_and_tags(text, source)
        node = yaml.compose(text, Loader=yaml.BaseLoader)
    except yaml.MarkedYAMLError as error:
        mark = error.problem_mark or error.context_mark
        where = f":{mark.line + 1}" if mark else ""
        raise ConfigError(
            f"{source}{where}: YAML syntax error: {error.problem or error}"
        ) from error
    except yaml.YAMLError as error:
        raise ConfigError(f"{source}: YAML error: {error}") from error
    if node is None:
        return Located(1)
    return _construct(node, source)


def value_text(value: object, field: str) -> str:
    """一个标量值的 C++ 文本，检查所有值共有的规则。
    The C++ text of a scalar value, after the checks every value shares.

    Raises:
        ConfigError: 值未填写、为空、以 @ 开头，或含有不允许的 C++ 写法。
            The value is not filled in, empty, starts with @, or holds C++ that is not
            allowed.
    """
    if value is None:
        raise ConfigError(
            f'{field} is not filled in (null, ~ and empty values mean "not filled in"; '
            "write nullptr for a null pointer)"
        )
    if not isinstance(value, str) or not value.strip():
        raise ConfigError(field + " requires C++ expression text")
    if value.lstrip().startswith("@"):
        raise ConfigError(
            f"{field}: the @ prefix of XRobot before 1.0 is gone; every value without quotes "
            "or in single quotes is C++ code, and a double-quoted value is a C++ string"
        )
    check_cpp_text(value, field)
    return value


def check_cpp_text(text: str, field: str) -> None:
    """检查 C++ 文本：值里不能有 C++ 注释、前导零的整数和非 ASCII 数字。
    Check C++ text: no C++ comments, integers with a leading zero or non-ASCII digits.

    Raises:
        ConfigError: 违反其中一条。
            One of these is found.
    """
    tokens = code_tokens(text)
    code = text
    for token in reversed([t for t in tokens if t.kind == "literal"]):
        code = code[: token.start] + " " * (token.end - token.start) + code[token.end :]
    if "//" in code or "/*" in code:
        raise ConfigError(
            f"{field}: C++ comments are not allowed inside a value; use a YAML # comment"
        )
    for token in tokens:
        if token.kind == "number" and re.fullmatch(r"0[0-9]+[uUlL]*", token.text):
            raise ConfigError(
                f"{field}: {token.text} has a leading zero, which C++ reads as octal; write the "
                "decimal value"
            )
    if any(ch.isdigit() and not ch.isascii() for ch in code):
        raise ConfigError(f"{field}: non-ASCII digits are not C++ numbers")


def identifier_problem(name: object) -> str | None:
    """name 不能作为生成的 C++ 对象名的原因；可以时为 None。
    Why name cannot be a generated C++ object name, or None when it can.
    """
    if not isinstance(name, str) or not re.fullmatch(IDENTIFIER, name):
        return "is not a C++ identifier"
    if name in CPP_KEYWORDS:
        return "is a C++ keyword"
    if name in RESERVED_MACROS:
        return "is a macro name"
    if name in RESERVED_NAMESPACES:
        return "is a reserved namespace"
    if name.startswith("xr_") or name.startswith("XR_") or name.startswith("xrobot_"):
        return "uses a prefix reserved for generated names"
    if name.startswith("__") or re.match(r"_[A-Z]", name):
        return "is reserved by the C++ standard"
    return None


def _check_value(value: object, field: str, errors: list[str]) -> None:
    """检查一个值及其子值，把错误追加到 errors。
    Check a value and its children, appending errors to errors.
    """
    if value is None:
        return  # A saved, unfilled configuration is valid; generation is not.
    if isinstance(value, dict):
        for name, child in value.items():
            if not re.fullmatch(IDENTIFIER, name):
                errors.append(f"{field}: invalid field name {name}")
                continue
            _check_value(child, field + "." + name, errors)
    elif isinstance(value, list):
        for i, child in enumerate(value):
            _check_value(child, f"{field}[{i}]", errors)
    else:
        try:
            value_text(value, field)
        except ConfigError as error:
            errors.append(str(error))


def _pre_1_0_format(config: dict) -> bool:
    """配置是否为 XRobot 1.0 以前的格式（global_settings、name/constructor_args）。
    Whether the configuration uses the format of XRobot before 1.0 (global_settings,
    name/constructor_args).
    """
    if "global_settings" in config:
        return True
    entries = config.get("modules")
    return isinstance(entries, list) and any(
        isinstance(entry, dict) and ("name" in entry or "constructor_args" in entry)
        for entry in entries
    )


def validate_config(config: object, source: str = "config") -> None:
    """检查配置的结构，一次列出全部错误。
    Check the structure of a configuration and report every error at once.

    Raises:
        ConfigError: 有任何错误；每行一条，带配置名前缀。
            Any error; one per line, prefixed with the configuration.
    """
    errors = []
    if not isinstance(config, dict):
        raise ConfigError(f"{source}: expected an application configuration mapping")
    if _pre_1_0_format(config):
        raise ConfigError(
            f"{source}: this configuration uses the format of XRobot before 1.0 "
            "(global_settings, name/constructor_args); XRobot 1.0 lists each instance as "
            "module, id and args; recreate the instances with `xrobot instance add`"
        )
    extra = [key for key in config if key not in TOP_LEVEL]
    if extra:
        errors.append(
            f"unknown top-level key(s) {', '.join(extra)}; allowed: {', '.join(TOP_LEVEL)}"
        )
    namespace = config.get("constexpr_namespace", "ProjectConstexpr")
    if not isinstance(namespace, str):
        errors.append("constexpr_namespace must be a C++ namespace name")
    elif not re.fullmatch(IDENTIFIER + "(?:::" + IDENTIFIER + ")*", namespace):
        errors.append(f"constexpr_namespace: {namespace} is not a C++ namespace name")
    else:
        for part in namespace.split("::"):
            problem = identifier_problem(part)
            if problem:
                errors.append(f"constexpr_namespace: {part} {problem}")
    includes = config.get("constexpr_includes", [])
    if not isinstance(includes, list):
        errors.append("constexpr_includes must be a list of header names")
    else:
        for i, header in enumerate(includes):
            if not isinstance(header, str):
                errors.append(
                    f"constexpr_includes[{i}] must be a header name such as Foo.hpp or <vector>"
                )
            elif not re.fullmatch(r'<[^<>"\s]+>|[^<>"\s]+', header.strip()):
                errors.append(
                    f"constexpr_includes[{i}]: {header} is not a header name such as Foo.hpp "
                    "or <vector>"
                )
    constants = config.get("constexprs", {})
    if not isinstance(constants, dict):
        errors.append("constexprs must be a mapping of name to {type, value}")
        constants = {}
    for name, spec in constants.items():
        problem = identifier_problem(name)
        if problem:
            errors.append(f"constexprs.{name} {problem}")
        if not isinstance(spec, dict) or set(spec) != {"type", "value"}:
            errors.append(f"constexprs.{name} requires exactly type and value")
            continue
        _check_value(spec["type"], f"constexprs.{name}.type", errors)
        _check_value(spec["value"], f"constexprs.{name}.value", errors)
    entries = config.get("modules", [])
    if not isinstance(entries, list):
        errors.append("modules must be an ordered list")
        entries = []
    used = set()
    for i, entry in enumerate(entries):
        where = f"modules[{i}]"
        if not isinstance(entry, dict):
            errors.append(f"{where} requires module/id and ordered args/template_args")
            continue
        unknown = [key for key in entry if key not in ("module", "id", "args", "template_args")]
        if unknown:
            errors.append(f"{where}: unknown key(s) {', '.join(unknown)}")
        for key in ("module", "id"):
            if not isinstance(entry.get(key), str) or not entry[key]:
                errors.append(f"{where}.{key} is required")
        identity = entry.get("id")
        if isinstance(identity, str) and identity:
            where = identity
            problem = identifier_problem(identity)
            if problem:
                errors.append(f"modules[{i}].id: {identity} {problem}")
            if identity in used:
                errors.append(f"modules[{i}].id: duplicate instance id {identity}")
            used.add(identity)
        values = entry.get("args", [])
        if not isinstance(values, list):
            errors.append(f"{where}.args must be an ordered list")
            values = []
        names = set()
        for j, value in enumerate(values):
            if not isinstance(value, dict) or len(value) != 1:
                errors.append(f"{where}.args[{j}] requires one named parameter")
                continue
            name, argument = next(iter(value.items()))
            if not re.fullmatch(IDENTIFIER, name) or name in names:
                errors.append(f"{where}.args[{j}]: invalid or duplicate parameter name {name}")
            names.add(name)
            _check_value(argument, f"{where}.args.{name}", errors)
        templates = entry.get("template_args", [])
        if not isinstance(templates, list):
            errors.append(f"{where}.template_args must be an ordered list")
            templates = []
        for j, value in enumerate(templates):
            if value is not None:
                _check_value(value, f"{where}.template_args[{j}]", errors)
    _check_settings(config.get("settings", {}), errors)
    if errors:
        raise ConfigError("\n".join(f"{source}: {error}" for error in errors))


def _check_settings(settings: object, errors: list[str]) -> None:
    """检查 settings：只有 monitor_sleep_ms，且是 32 位无符号十进制数。
    Check settings: only monitor_sleep_ms, an unsigned 32-bit decimal number.
    """
    if not isinstance(settings, dict):
        errors.append("settings must be a mapping")
        return
    unknown = [key for key in settings if key != "monitor_sleep_ms"]
    if unknown:
        errors.append(
            f"settings: unknown key(s) {', '.join(unknown)}; the only setting is monitor_sleep_ms"
        )
    sleep = settings.get("monitor_sleep_ms", "1000")
    if not isinstance(sleep, str):
        errors.append(
            "settings.monitor_sleep_ms must be an unsigned 32-bit decimal millisecond count"
        )
    elif not re.fullmatch(r"0|[1-9][0-9]*", sleep) or int(sleep) > 0xFFFFFFFF:
        errors.append(
            f"settings.monitor_sleep_ms: {sleep} is not an unsigned 32-bit decimal "
            "millisecond count"
        )


def load_config(path: str | Path, source: str | None = None) -> "Located":
    """读取并检查一份应用配置。
    Read and check an application configuration.

    Raises:
        ConfigError: 文件不是 UTF-8、YAML 有误，或结构不对。
            The file is not UTF-8, the YAML is invalid, or the structure is wrong.
    """
    path = Path(path)
    source = source or path.as_posix()
    try:
        text = path.read_text(encoding="utf-8-sig")
    except UnicodeDecodeError as error:
        raise ConfigError(f"{source}: not UTF-8 text") from error
    config = parse_yaml(text, source)
    validate_config(config, source)
    return config
