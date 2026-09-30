"""模块构造函数的约定、构造函数的选择，以及配置值到 C++ 初始化的渲染。
Constructor contracts of Modules, constructor selection, and the rendering of
configuration values as C++ initializers.

只支持带名字的显式声明，不是 C++ 类型系统：配置的树形结构来自初始化器写法；YAML 映射对照
已加载模块头文件中的类定义（xrobot.type_index）检查，索引找不到的类型则对照参数默认值中的
指定初始化器检查。只有确定有错时生成器才拒绝一个值，其余交给 C++ 编译器。
Only named, explicit declarations are supported; this is not a C++ type system.
Configuration trees come from initializer expressions, and a YAML mapping is checked
against the class definition read from a loaded Module header (xrobot.type_index) or, for
a type the index cannot locate, against the parameter's designated default initializer.
The generator rejects a value only when the problem is certain; everything else is left
to the C++ compiler.
"""

from __future__ import annotations

import re

from xrobot.config import ConfigError, value_text
from xrobot.source_syntax import close_token, code_tokens, split_arguments
from xrobot.type_index import ClassEntry, TypeIndex, class_scope_names

ARITHMETIC = frozenset(
    [
        "bool",
        "char",
        "wchar_t",
        "char8_t",
        "char16_t",
        "char32_t",
        "short",
        "int",
        "long",
        "float",
        "double",
        "signed",
        "unsigned",
        "int8_t",
        "int16_t",
        "int32_t",
        "int64_t",
        "uint8_t",
        "uint16_t",
        "uint32_t",
        "uint64_t",
        "size_t",
        "ptrdiff_t",
        "intptr_t",
        "uintptr_t",
    ]
)

Tree = dict | list | str | None


def parameter(declaration: str) -> dict:
    """把一个参数声明拆成名字、类型和默认值。
    Split one parameter declaration into its name, type and default.

    Returns:
        含 name、type、default（没有时为 None）和 declaration 的映射。
        A mapping with name, type, default (None when absent) and declaration.

    Raises:
        ValueError: 参数没有名字、= 后没有默认值，或类型需要先定义别名（数组、函数指针、
            参数包）。
            The parameter has no name or nothing after its =, or its type needs an alias
            first (arrays, function pointers, packs).
    """
    items = code_tokens(declaration)
    split = next((t for t in items if t.text == "="), None)
    head = declaration[: split.start].strip() if split else declaration.strip()
    default = declaration[split.end :].strip() if split else None
    if default == "":
        raise ValueError("Missing default value after '=': " + declaration)
    parts = code_tokens(head)
    if not parts or parts[-1].kind != "identifier" or len(parts) < 2:
        raise ValueError("Constructor parameters must have explicit names: " + declaration)
    name = parts[-1].text
    cpp_type = head[: parts[-1].start].strip()
    # 类型模板参数也用同样的带名字声明表示。
    # Type template parameters use the same named declaration representation.
    if name in ("const", "volatile") and cpp_type not in ("class", "typename"):
        raise ValueError("Unsupported parameter declaration: " + declaration)
    if not cpp_type or "(" in cpp_type or "[" in cpp_type or "..." in head:
        raise ValueError("Use an explicit named type alias for this declaration: " + declaration)
    return {"name": name, "type": cpp_type, "default": default, "declaration": declaration}


def enrich_interface(source: str, interface: dict, source_name: str | None = None) -> dict:
    """给模块接口补上可见名字、别名、解析后的构造参数和模板参数。
    Add the visible names, aliases, parsed constructor arguments and template parameters
    to a Module interface.

    可见名字由类型索引读取类体的方式得出（xrobot.type_index.class_scope_names）。
    The visible names come from the way the type index reads class bodies
    (xrobot.type_index.class_scope_names).

    Args:
        source_name: 源文件名；与解析接口时相同，以共用一次解析。
            The source file name; the same as when the interface was extracted, so both
            share one parse.
    """
    interface["symbols"], interface["aliases"] = class_scope_names(
        source, interface["name"], source_name
    )
    for ctor in interface["constructors"]:
        declarations = ctor["parameters"]
        if declarations == ["void"]:
            declarations = []
        ctor["arguments"] = [parameter(p) for p in declarations]
    interface["template_parameters"] = [parameter(p) for p in interface["template_declarations"]]
    return interface


def replace_names(text: str, replacements: dict[str, str]) -> str:
    """替换 text 中未加限定的名字。
    Replace the unqualified names of text.

    作用域的根名（如 Mode::VALUE 中的 Mode、T::value_type 中的 T）也会被替换。
    Scope roots such as Mode in Mode::VALUE and T in T::value_type are replaced too.
    """
    items = code_tokens(text)
    edits = []
    for i, token in enumerate(items):
        previous = items[i - 1].text if i else ""
        if (
            token.kind == "identifier"
            and token.text in replacements
            and previous not in (".", "->", ".*", "->*", "::")
        ):
            edits.append((token.start, token.end, replacements[token.text]))
    for start, end, replacement in reversed(edits):
        text = text[:start] + replacement + text[end:]
    return text


def template_bindings(interface: dict, supplied: list) -> dict[str, str]:
    """模板参数名到模板实参的映射；没有给出的取默认值。
    The template arguments by template parameter name; missing ones take their default.

    Raises:
        ValueError: 实参过多，或缺少没有默认值的实参。
            Too many arguments, or an argument without a default is missing.
    """
    parameters = interface["template_parameters"]
    if len(supplied) > len(parameters):
        raise ValueError("Too many template arguments for " + interface["name"])
    replacements = {}
    for i, p in enumerate(parameters):
        value = supplied[i] if i < len(supplied) else p["default"]
        if value is None:
            raise ValueError(f"Template argument {interface['name']}.{p['name']} must be specified")
        replacements[p["name"]] = replace_names(str(value), replacements)
    return replacements


def qualify(
    text: str | None,
    interface: dict,
    cpp_class: str,
    templates: dict[str, str] | None = None,
    expand_aliases: bool = False,
) -> str | None:
    """给 text 中的模块类成员名加上 cpp_class:: 限定，并代入模板实参。
    Qualify the Module class members named in text with cpp_class:: and substitute the
    template arguments.

    Args:
        expand_aliases: 把公有成员别名替换为它指向的写法。
            Replace public member aliases with the spellings they stand for.

    Raises:
        ValueError: text 用到了非公有成员。
            text uses a member that is not public.
    """
    if text is None:
        return None
    substitutions = dict(templates or {})
    for symbol, access in interface["symbols"].items():
        if access == "public":
            substitutions[symbol] = cpp_class + "::" + symbol
    # 只有别名的写法可读时才展开。
    # Aliases are followed only when their spelling is available.
    if expand_aliases:
        for _ in range(len(interface["aliases"]) + 1):
            altered = False
            for symbol, value in interface["aliases"].items():
                if interface["symbols"].get(symbol) != "public":
                    continue
                target = replace_names(
                    value, {k: v for k, v in substitutions.items() if k != symbol}
                )
                if substitutions.get(symbol) != target:
                    substitutions[symbol] = target
                    altered = True
            if not altered:
                break
    items = code_tokens(text)
    for i, token in enumerate(items):
        if interface["symbols"].get(token.text) not in ("private", "protected"):
            continue
        previous = items[i - 1].text if i else ""
        own_scope = previous == "::" and i >= 2 and items[i - 2].text == interface["name"]
        if previous not in (".", "->", ".*", "->*", "::") or own_scope:
            raise ValueError(f"Expression uses non-public member {interface['name']}::{token.text}")
    return replace_names(text, substitutions)


def initializer_tree(expression: str | None, expected_type: str | None = None) -> Tree:
    """展开花括号初始化器中显式写出的项，不查看类型的字段。
    Expand the explicit entries of a brace initializer without looking at a type's fields.

    Returns:
        指定初始化器为映射，按位置的初始化器为列表，其他写法（常量名、工厂函数、转换）为
        原文。
        A mapping for a designated initializer, a list for a positional one, and the
        original text for anything else (named constants, factory calls, casts).

    Raises:
        ValueError: 指定初始化器中字段重复。
            A designated initializer repeats a field.
    """
    if expression is None:
        return None
    ts = code_tokens(expression)
    if not ts:
        return expression
    opening = next((i for i, t in enumerate(ts) if t.text == "{"), None)
    if opening is None or close_token(ts, opening) != len(ts) - 1:
        return expression
    prefix = expression[: ts[opening].start].strip()
    if prefix:
        if (
            expected_type is None
            or type_shape(prefix)[0] != type_shape(expected_type)[0]
            or type_shape(prefix)[2]
            or type_shape(expected_type)[2]
        ):
            return expression
        if any(t.text in ("[", "]", "(", ")", "=", "?", "+", "-") for t in ts[:opening]):
            return expression
    body = expression[ts[opening].end : ts[-1].start].strip().rstrip(",").strip()
    if not body:
        return []
    parts = split_arguments(body)
    fields = []
    for item in parts:
        its = code_tokens(item)
        if (
            len(its) >= 3
            and its[0].text == "."
            and its[1].kind == "identifier"
            and its[2].text == "="
        ):
            fields.append((its[1].text, initializer_tree(item[its[2].end :].strip())))
        else:
            fields.append(None)
    if all(f is not None for f in fields):
        result = {}
        for name, value in fields:
            if name in result:
                raise ValueError("Duplicate initializer field: " + name)
            result[name] = value
        return result
    if any(f is not None for f in fields):
        return expression
    return [initializer_tree(p) for p in parts]


def initializer_text(tree: Tree) -> str | None:
    """把 initializer_tree 的结果写回 C++ 初始化器。
    Write a result of initializer_tree back as a C++ initializer.
    """
    if isinstance(tree, dict):
        return "{" + ", ".join(f".{k} = {initializer_text(v)}" for k, v in tree.items()) + "}"
    if isinstance(tree, list):
        return "{" + ", ".join(initializer_text(v) or "" for v in tree) + "}"
    return tree


def compliant_constructors(interface: dict) -> list[dict]:
    """符合约定的构造函数：没有默认值的依赖在前，带默认值的值在后。
    The constructors that follow the agreed shape: dependencies without defaults first,
    then values with defaults.

    Raises:
        ValueError: 没有符合约定的构造函数；报错列出每个构造函数的问题。
            No constructor follows it; the message lists each constructor's problem.
    """
    accepted, rejected = [], []
    for ctor in interface["constructors"]:
        config_started = False
        problems = []
        for p in ctor["arguments"]:
            if p["default"] is not None:
                config_started = True
            elif config_started:
                problems.append(
                    f"{p['name']}: dependency without a default appears after value configuration"
                )
        if problems:
            rejected.append(f"line {ctor.get('line', '?')}: {'; '.join(problems)}")
        else:
            accepted.append(ctor)
    if not accepted:
        raise ValueError(f"{interface['name']}: no compliant constructor; {' | '.join(rejected)}")
    return accepted


def type_shape(cpp_type: str) -> tuple[str, frozenset, tuple, str]:
    """拆出类型写法最外层的 cv、指针和引用，不拆模板实参。
    Split the outer cv qualifiers, pointers and reference of a type spelling, never the
    template arguments.

    Returns:
        (去掉空白的基本类型, 基本类型的 cv, 每层指针的 cv, 引用 '&'/'&&'/'')。
        (base type without whitespace, cv of the base type, cv of each pointer level,
        reference '&'/'&&'/'').
    """
    text = cpp_type.strip()
    items = code_tokens(text)
    outer = []
    i = 0
    while i < len(items):
        if items[i].text in ("<", "(", "[", "{"):
            i = close_token(items, i) + 1
        else:
            outer.append(items[i])
            i += 1
    reference = ""
    if outer and outer[-1].text in ("&", "&&"):
        reference = outer[-1].text
        text = text[: outer.pop().start].rstrip()
    stars = [t for t in outer if t.text == "*"]
    base_end = stars[0].start if stars else len(text)
    qualifiers = [
        t for t in outer if t.start < base_end and t.text in ("const", "volatile", "typename")
    ]
    base = text[:base_end]
    for token in reversed(qualifiers):
        base = base[: token.start] + base[token.end :]
    base = re.sub(r"\s+", "", base)
    cv = frozenset(t.text for t in qualifiers if t.text != "typename")
    pointers = []
    for i, star in enumerate(stars):
        end = stars[i + 1].start if i + 1 < len(stars) else len(text)
        pointers.append(
            frozenset(
                t.text
                for t in outer
                if star.end <= t.start < end and t.text in ("const", "volatile")
            )
        )
    return base, cv, tuple(pointers), reference


def is_arithmetic(cpp_type: str) -> bool:
    """按值传递的内置算术类型，含 <cstdint>/<cstddef> 中的写法。
    Whether a type is a builtin arithmetic type passed by value, <cstdint>/<cstddef>
    spellings included.
    """
    base, _, pointers, reference = type_shape(cpp_type)
    if pointers or reference:
        return False
    words = [t.text for t in code_tokens(cpp_type) if t.text not in ("const", "volatile", "::")]
    if words and words[0] == "std":
        words = words[1:]
    return bool(words) and all(w in ARITHMETIC for w in words)


def is_dependency(p: dict) -> bool:
    """是否为依赖参数：没有默认值的引用或指针。
    Whether a parameter is a dependency: a reference or pointer without a default.
    """
    _, _, pointers, reference = type_shape(p["type"])
    return p["default"] is None and bool(pointers or reference)


def binding_candidates(
    index: TypeIndex | None, target: str, named: dict[str, str], dependency: bool = True
) -> list[str]:
    """参数可以写的名字：类型与 target 相同或能确定公有派生自它的对象。
    The names a parameter can take: objects whose type is target's or certainly derives
    publicly from it.

    指针参数的候选写成 &名字；依赖指针最后加上 nullptr（不使用这个可选依赖）。
    Candidates of a pointer parameter are written &name; a dependency pointer also gets
    nullptr last (the optional dependency is not used).

    Args:
        named: 名字到对象类型的映射（注册名、排在前面的实例），按候选顺序。
            Object types by name (registrations, earlier instances), in candidate order.
    """
    base, _, pointers, _ = type_shape(target)
    result = []
    for name, cpp_type in named.items():
        source, _, source_pointers, _ = type_shape(cpp_type)
        if source != base and not (index is not None and index.derives_from(source, base)):
            continue
        if len(source_pointers) == len(pointers):
            result.append(name)
        elif not source_pointers and len(pointers) == 1:
            result.append("&" + name)
    if dependency and len(pointers) == 1:
        result.append("nullptr")
    return result


def explicit_expression_type(value: object) -> str | None:
    """值的写法本身给出的类型：显式转换、带类型的花括号，以及少量可移植的字面量。
    The type a value's spelling states itself: an explicit cast, a typed brace
    initializer, or one of a few portable literals.
    """
    if not isinstance(value, str):
        return None
    value = value.strip()
    if value in ("true", "false"):
        return "bool"
    ts = code_tokens(value)
    if (
        len(ts) >= 5
        and ts[0].text in ("static_cast", "const_cast", "reinterpret_cast", "dynamic_cast")
        and ts[1].text == "<"
    ):
        ending = close_token(ts, 1)
        if (
            ending + 1 < len(ts)
            and ts[ending + 1].text == "("
            and close_token(ts, ending + 1) == len(ts) - 1
        ):
            return value[ts[1].end : ts[ending].start]
    opening = next((i for i, t in enumerate(ts) if t.text == "{"), None)
    if opening and close_token(ts, opening) == len(ts) - 1:
        prefix = value[: ts[opening].start].strip()
        if re.fullmatch(r"[A-Za-z_][A-Za-z_0-9:]*(?:\s*<[^{};]+>)?", prefix):
            return prefix
    if re.fullmatch(r"[+-]?\d+", value) and -32767 <= int(value) <= 32767:
        return "int"
    if re.fullmatch(r"[+-]?(?:\d+\.\d*|\d*\.\d+|\d+[eE][+-]?\d+)(?:[eE][+-]?\d+)?[fFlL]?", value):
        return (
            "float"
            if value[-1:] in ("f", "F")
            else "long double"
            if value[-1:] in ("l", "L")
            else "double"
        )
    return None


def constructor_for(
    interface: dict,
    named_values: list[dict],
    known: dict[str, str],
    cpp_class: str,
    templates: dict[str, str] | None,
) -> dict:
    """按配置列出的参数名选择构造函数。
    Select the constructor whose parameter names the configuration lists.

    参数名相同的几个构造函数只按显式类型区分：已知名字的类型、显式转换、带类型的花括号或
    字面量。
    Several constructors with the same names are told apart by explicit types only: the
    type of a known name, a cast, a typed brace initializer or a literal.

    Args:
        known: 注册名和排在前面的实例 id 到其类型的映射。
            The types of the registration names and earlier instance ids.

    Raises:
        ValueError: 没有参数名匹配的构造函数，或显式类型不足以区分。
            No constructor has these names, or the explicit types cannot tell them apart.
    """
    names = [next(iter(v)) for v in named_values]
    candidates = []
    supported = compliant_constructors(interface)
    for ctor in supported:
        if [p["name"] for p in ctor["arguments"]] != names:
            continue
        good = True
        for p, item in zip(ctor["arguments"], named_values, strict=False):
            value = next(iter(item.values()))
            target = qualify(p["type"], interface, cpp_class, templates, True)
            text = value.strip() if isinstance(value, str) else None
            if text is not None and text.lstrip("&") in known:
                source = known[text.lstrip("&")]
                if text.startswith("&"):
                    source += "*"
                if type_shape(source)[0] != type_shape(target)[0]:
                    good = False
                continue
            known_type = explicit_expression_type(value)
            if known_type is not None:
                source = qualify(known_type, interface, cpp_class, templates, True)
                if (
                    type_shape(source)[0] != type_shape(target)[0]
                    or type_shape(source)[2] != type_shape(target)[2]
                ):
                    good = False
        candidates.append((ctor, good))
    if len(candidates) == 1:
        return candidates[0][0]
    typed = [ctor for ctor, good in candidates if good]
    if len(typed) == 1:
        return typed[0]
    if not candidates:
        expected = " | ".join(
            "(" + ", ".join(p["name"] for p in c["arguments"]) + ")" for c in supported
        )
        raise ValueError(
            f"named arguments ({', '.join(names)}) do not match any constructor of "
            f"{interface['name']}; expected one of: {expected}"
        )
    raise ValueError(
        f"{interface['name']}: constructor is ambiguous for the supplied names and explicit types"
    )


def _base_spelling(cpp_type: str) -> str:
    """去掉类型写法最外层的 cv 和引用，保留模板实参。
    Drop the outer cv qualifiers and reference of a type spelling, keeping template
    arguments.
    """
    items = code_tokens(cpp_type)
    keep = [t for t in items if t.text not in ("const", "volatile", "&", "&&")]
    if not keep:
        return cpp_type.strip()
    return cpp_type[keep[0].start : keep[-1].end].strip()


def _is_positional_brace(text: str) -> bool:
    """text 是否为有元素的按位置花括号：{a, b} 或 T{a, b}。
    Whether text is a positional brace initializer with elements: {a, b} or T{a, b}.
    """
    ts = code_tokens(text)
    opening = next((i for i, t in enumerate(ts) if t.text == "{"), None)
    if opening is None or close_token(ts, opening) != len(ts) - 1:
        return False
    prefix = text[: ts[opening].start].strip()
    if prefix and not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9:]*(?:\s*<.*>)?", prefix, re.S):
        return False
    inner = ts[opening + 1 : -1]
    return bool(inner) and not (len(inner) >= 2 and inner[0].text == ".")


def _is_designated_brace(text: str) -> bool:
    """text 是否为指定初始化器：{.a = 1} 或 T{.a = 1}。
    Whether text is a designated initializer: {.a = 1} or T{.a = 1}.
    """
    ts = code_tokens(text)
    opening = next((i for i, t in enumerate(ts) if t.text == "{"), None)
    return (
        opening is not None
        and opening + 1 < len(ts)
        and ts[opening + 1].text == "."
        and close_token(ts, opening) == len(ts) - 1
    )


class ValueChecker:
    """按映射规则把 YAML 值写成 C++。
    Render YAML values as C++ while enforcing the mapping rules.

    类型在已加载的模块头文件中时，映射必须按顺序写出它的全部数据成员（聚合体）或某个公有
    构造函数的全部参数（类），按位置的列表和花括号会被拒绝。类型不在索引中时，映射必须与参数
    默认值中的指定初始化器一致，否则被拒绝，需要写成 C++ 表达式。
    For a type located in the loaded Module headers a mapping must name exactly its data
    members in order (aggregates) or one public constructor's parameters (classes);
    positional lists and positional brace text are rejected. For a type the index cannot
    locate, a mapping must match the parameter's designated default initializer, otherwise
    it is rejected and the value must be written as a C++ expression.
    """

    def __init__(self, index: TypeIndex | None) -> None:
        """在 index 中查找类型；index 为 None 时每个类型都按找不到处理。
        Look types up in index; with None every type counts as not located.
        """
        self.index = index
        self.checks: list[str] = []  # static_assert declarations for the value being rendered

    def _locate(self, cpp_type: str | None, scope: tuple[str, ...]) -> ClassEntry | None:
        """类型写法指向的已索引的类。
        The indexed class a type spelling names.
        """
        if not (self.index and cpp_type):
            return None
        return self.index.resolve(cpp_type, scope)

    def render(
        self,
        value: object,
        field: str,
        cpp_type: str | None = None,
        scope: tuple[str, ...] = (),
        default: Tree = None,
    ) -> tuple[str, bool]:
        """一个配置值的 C++ 表达式。
        The C++ expression of one configuration value.

        Returns:
            (表达式, 是否已是带类型的表达式)。
            (expression, whether it is already a typed expression).

        Raises:
            ConfigError: 值不符合映射规则。
                The value breaks a mapping rule.
        """
        if isinstance(value, dict):
            return self._mapping(value, field, cpp_type, scope, default)
        entry = self._locate(cpp_type, scope) if cpp_type else None
        if isinstance(value, list):
            if entry is not None:
                raise ConfigError(
                    f"{field}: positional values are not accepted for {entry.qualified}; write a mapping "
                    "with its field names"
                )
            items = (
                default
                if isinstance(default, list) and len(default) == len(value)
                else [None] * len(value)
            )
            parts = [
                self.render(v, f"{field}[{i}]", None, scope, d)[0]
                for i, (v, d) in enumerate(zip(value, items, strict=True))
            ]
            return ("{\n" + "\n, ".join(parts) + "\n}" if parts else "{}"), False
        text = value_text(value, field)
        if entry is not None:
            if _is_designated_brace(text):
                tree = initializer_tree(text, cpp_type)
                if isinstance(tree, dict):
                    return self._mapping(tree, field, cpp_type, scope, default)
            if _is_positional_brace(text):
                raise ConfigError(
                    f"{field}: positional initializer {text.strip()} is not accepted for {entry.qualified}; write a "
                    "mapping with its field names"
                )
        return text, False

    @staticmethod
    def _require(field: str, keys: list[str], expected: list[str], what: str) -> None:
        """映射的键必须与 expected 相同且顺序一致。
        The keys of a mapping must equal expected, in the same order.

        Raises:
            ConfigError: 缺少、多出或顺序不对。
                A key is missing, unknown or out of order.
        """
        if keys == expected:
            return
        missing = [k for k in expected if k not in keys]
        extra = [k for k in keys if k not in expected]
        if missing or extra:
            detail = []
            if missing:
                detail.append("missing " + ", ".join(missing))
            if extra:
                detail.append("unknown " + ", ".join(extra))
            raise ConfigError(
                f"{field}: {'; '.join(detail)} ({what}); expected: {', '.join(expected)}"
            )
        raise ConfigError(
            f"{field}: fields out of declaration order ({what}); expected: {', '.join(expected)}"
        )

    def _mapping(
        self,
        value: dict,
        field: str,
        cpp_type: str | None,
        scope: tuple[str, ...],
        default: Tree,
    ) -> tuple[str, bool]:
        """一个映射值的 C++ 表达式：聚合体写成指定初始化器，有构造函数的类写成构造调用。
        The C++ expression of a mapping value: a designated initializer for an aggregate, a
        constructor call for a class with constructors.

        Raises:
            ConfigError: 映射与类型或默认值不符，或类型无法检查。
                The mapping does not match the type or the default, or the type cannot be
                checked.
        """
        for key in value:
            if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", str(key)):
                raise ConfigError(field + ": invalid field name " + str(key))
        keys = list(value)
        entry = self._locate(cpp_type, scope)
        spelled = _base_spelling(cpp_type) if cpp_type else None
        if entry is None:
            if isinstance(default, dict):
                self._require(field, keys, list(default), "from the default initializer")
                return self._designated(value, field, {}, None, default), False
            raise ConfigError(
                f"{field}: cannot verify the fields of {cpp_type or 'this value'} in the loaded Module headers; "
                "write this value as a complete C++ expression"
            )
        problem = entry.mapping_problem()
        if problem:
            raise ConfigError(f"{field}: {problem}; write this value as a complete C++ expression")
        if entry.is_aggregate():
            fields = entry.fields()
            self._require(
                field, keys, [n for n, _, _ in fields], "data members of " + entry.qualified
            )
            types = {n: self.index.qualify_in(t, entry, spelled) for n, t, _ in fields}
            child_defaults = default if isinstance(default, dict) else None
            return self._designated(value, field, types, entry, child_defaults), False
        ctors = [c for c in entry.constructors() if c]
        chosen = [c for c in ctors if [p["name"] for p in c] == keys]
        if len(chosen) != 1:
            options = " | ".join(", ".join(p["name"] for p in c) for c in ctors) or "none"
            raise ConfigError(
                f"{field}: {entry.qualified} has constructors; the mapping must name one constructor's "
                f"parameters in order. Constructors: {options}"
            )
        args = []
        for p in chosen[0]:
            param_type = self.index.qualify_in(p["type"], entry, spelled)
            child = value[p["name"]]
            expr, typed = self.render(child, field + "." + p["name"], param_type, entry.path, None)
            args.append(
                convert(
                    expr,
                    param_type,
                    typed or isinstance(child, (dict, list)),
                    self.checks,
                    field + "." + p["name"],
                )[0]
            )
        joined = "\n, ".join(args)
        return f"{spelled}(\n{joined}\n)", True

    def _designated(
        self,
        value: dict,
        field: str,
        types: dict[str, str],
        entry: ClassEntry | None,
        default: Tree,
    ) -> str:
        """把映射写成指定初始化器 {.a = …}。
        Write a mapping as a designated initializer {.a = ...}.
        """
        parts = []
        for key, child in value.items():
            child_default = default.get(key) if isinstance(default, dict) else None
            child_type = types.get(key)
            scope = entry.path if entry is not None else ()
            expr, _ = self.render(child, field + "." + key, child_type, scope, child_default)
            parts.append(f".{key} = {expr}")
        return "{\n" + "\n, ".join(parts) + "\n}" if parts else "{}"


def convert(
    expr: str, target: str, exact: bool, checks: list[str], message: str = "value"
) -> tuple[str, list[str]]:
    """按生成器的转换规则写出一个值。
    Apply the generator's conversion rule to one value.

    花括号写成 std::remove_cv_t<std::remove_reference_t<T>>{…}；生成器构造的带类型表达式原样
    返回；算术类型经 Implicit<P>，保留编译器对常量转换的警告；其他类型加隐式可转换检查和
    static_cast，保留纯右值省略，拒绝向下转换和只能显式的转换。
    A brace initializer becomes std::remove_cv_t<std::remove_reference_t<T>>{...}; a
    typed expression the generator built is returned unchanged; an arithmetic target goes
    through Implicit<P>, so constant conversions keep the compiler's warnings; every other
    target gets an implicit-convertibility check and a static_cast, which keeps prvalue
    elision and rejects downcasts and explicit-only conversions.

    Returns:
        (表达式, 追加了检查的 checks)。
        (expression, checks with any new check appended).
    """
    if expr.lstrip().startswith("{"):
        return f"std::remove_cv_t<std::remove_reference_t<{target}>>{expr}", checks
    if exact:
        return expr, checks
    if is_arithmetic(target):
        return f"xrobot_generated::Implicit<{_base_spelling(target)}>({expr})", checks
    # 同一类型（如不可移动的工厂函数纯右值）不需要转换。
    # The same type (e.g. an immovable factory prvalue) needs no conversion.
    quoted_target = target.replace('"', "'")
    checks.append(
        f"static_assert(std::is_same_v<std::remove_cvref_t<decltype(({expr}))>, "
        f"std::remove_cvref_t<{target}>> ||\n"
        f"              std::is_convertible_v<decltype(({expr})), {target}>,\n"
        f'              "{message} requires an implicit conversion to {quoted_target}");'
    )
    return f"static_cast<{target}>({expr})", checks
