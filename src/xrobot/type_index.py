"""按名字查找模块头文件中的 class/struct，读取字段顺序、默认值与公有构造函数。
Find class/struct definitions in loaded Module headers by name, and read their data
members in declaration order, their defaults and their public constructors.

配置中映射写法的完整性检查依据这些信息，不对类型求值。一个名字可能指向索引看不到的东西
（外层类模板的模板参数、不在已加载头文件中的基类成员）时，结果是"未知"，不会退回到外层
同名的无关声明。头文件按需解析：只解析文本中出现所查名字的文件，每个文件只解析一次。
The completeness checks of mapping values rest on this information; types are never
evaluated. A name that may denote something the index cannot see (a template parameter
of an enclosing class template, a member of a base class outside the loaded headers) is
reported as unknown instead of falling back to an unrelated outer declaration with the
same name. Headers are parsed on demand: only files whose text mentions a requested name,
each once.
"""

from __future__ import annotations

import bisect
import functools
import os
import re
from collections.abc import Iterable, Sequence
from pathlib import Path

from xr_syntax.cpp import CppClassView

from xrobot.source_syntax import (
    Token,
    close_token,
    code_tokens,
    parse_document,
    split_arguments,
)

_SCOPES = ("namespace_definition", "class_specifier", "struct_specifier")
_CONDITIONALS = ("preproc_if", "preproc_ifdef", "preproc_call")
_WANTED = frozenset(_SCOPES + _CONDITIONALS + ("preproc_def", "compound_statement"))
_CLASS_KEYS = ("class", "struct", "union", "enum")
_SKIP_LEADING = {
    "using",
    "typedef",
    "friend",
    "template",
    "static_assert",
    "operator",
    "~",
    "public",
    "protected",
    "private",
}
_SPECIFIERS = {
    "static",
    "inline",
    "constexpr",
    "consteval",
    "constinit",
    "mutable",
    "thread_local",
    "volatile",
    "const",
    "explicit",
    "virtual",
}
_ATTRIBUTES = r"(?:\[\[[^\]]*\]\]\s*|alignas\s*\([^)]*\)\s*)*"
_QUOTED_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)


def _strip_type(text: str) -> tuple[str, ...] | None:
    """类类型写法中的名字路径；指针、数组和函数类型为 None。
    The name path of a class type spelling; None for pointer, array and function types.
    """
    items = code_tokens(text)
    names, current, i = [], None, 0
    while i < len(items):
        token = items[i]
        if token.text in ("const", "volatile", "typename", "struct", "class", "&", "&&"):
            i += 1
            continue
        if token.text in ("*", "[", "("):
            return None
        if token.text == "<":
            i = close_token(items, i) + 1
            continue
        if token.text == "::":
            if current is not None:
                names.append(current)
            current = None
            i += 1
            continue
        if token.kind == "identifier":
            current = token.text
        i += 1
    if current is not None:
        names.append(current)
    return tuple(names) if names else None


def _split_last(spelled: str) -> tuple[str, list[str] | None]:
    """把 A<x>::B<y, z> 拆成外层写法 A<x> 和最后一段的模板实参 ['y', 'z']。
    Split A<x>::B<y, z> into the outer spelling A<x> and the last part's template
    arguments ['y', 'z'].
    """
    items = code_tokens(spelled)
    cut, i, last_open = None, 0, None
    while i < len(items):
        if items[i].text == "<":
            last_open = i
            i = close_token(items, i) + 1
            continue
        if items[i].text == "::":
            cut = items[i].start
            last_open = None
        i += 1
    outer = spelled[:cut].strip() if cut is not None else ""
    args = None
    if last_open is not None and (cut is None or items[last_open].start > cut):
        close = close_token(items, last_open)
        args = split_arguments(spelled[items[last_open].end : items[close].start])
    return outer, args


def _replace_identifiers(text: str, replacements: dict[str, str]) -> str:
    """替换 text 中未加限定的标识符。
    Replace the unqualified identifiers of text.
    """
    items = code_tokens(text)
    edits = []
    for i, token in enumerate(items):
        if token.kind != "identifier" or token.text not in replacements:
            continue
        if i and items[i - 1].text in ("::", ".", "->"):
            continue
        edits.append((token.start, token.end, replacements[token.text]))
    for start, end, value in reversed(edits):
        text = text[:start] + value + text[end:]
    return text


def _directive_words(node) -> list[str]:
    """预处理指令节点中的词，如 ['#', 'ifndef', 'LED_HPP']。
    The words of a preprocessor directive node, e.g. ['#', 'ifndef', 'LED_HPP'].
    """
    return [child.text for child in node.syntax_children if not child.is_trivia]


def _is_include_guard(deltas: list, defines: list, tokens: Sequence[Token]) -> bool:
    """第一个条件指令和最后一个 #endif 是否构成包住全部代码的 include guard。
    Whether the first conditional directive and the last #endif form an include guard
    (#ifndef X, #define X, ..., #endif) around all code of the file.

    Args:
        deltas: 按位置排序的 (字节位置, +1/-1, 节点)，+1 为 #if/#ifdef/#ifndef，-1 为 #endif。
            (byte position, +1/-1, node) sorted by position: +1 for #if/#ifdef/#ifndef,
            -1 for #endif.
        defines: 文件中的 #define 节点。
            The #define nodes of the file.
        tokens: 文件的代码 token。
            The code tokens of the file.
    """
    if len(deltas) < 2 or deltas[0][1] != 1 or deltas[0][2].kind != "preproc_ifdef":
        return False
    words = _directive_words(deltas[0][2])
    if words[1:2] != ["ifndef"] or len(words) < 3:
        return False
    start, end = deltas[0][0], deltas[-1][0]
    following = [node for node in defines if node.span.start > start]
    if not following:
        return False
    define = min(following, key=lambda node: node.span.start)
    if _directive_words(define)[2:3] != [words[2]]:
        return False
    if tokens and (tokens[0].span.start < start or tokens[-1].span.start > end):
        return False
    depth = 0
    for i, (_, delta, _) in enumerate(deltas):
        depth += delta
        if depth == 0:
            return i == len(deltas) - 1
    return False


class _Header:
    """一个已解析的头文件：文本、语法文档和代码 token。
    One parsed header: its text, syntax document and code tokens.

    语法节点的 span 是字节位置；token 的 start/end 是字符位置，span 是字节位置。
    Syntax-node spans are byte offsets; a token's start/end are character offsets and its
    span is a byte range.
    """

    def __init__(self, path: Path | None, text: str) -> None:
        self.path = path
        self.text = text
        self.document = parse_document(text, str(path) if path is not None else None)
        self.tokens = code_tokens(text)
        self._starts = [token.span.start for token in self.tokens]
        self.scopes: list[tuple[int, int, str]] | None = None
        # 一次遍历收集索引需要的全部节点。
        # One tree walk collects every node kind the index needs.
        self.nodes: dict[str, list] = {}
        for element in self.document.root.descendants():
            if element.kind in _WANTED and hasattr(element, "child_by_field"):
                self.nodes.setdefault(element.kind, []).append(element)
        deltas = []
        for kind in _CONDITIONALS:
            for node in self.nodes.get(kind, []):
                if kind != "preproc_call":
                    deltas.append((node.span.start, 1, node))
                elif _directive_words(node)[1:2] == ["endif"]:
                    deltas.append((node.span.start, -1, node))
        deltas.sort(key=lambda delta: delta[0])
        if _is_include_guard(deltas, self.nodes.get("preproc_def", []), self.tokens):
            deltas = deltas[1:-1]
        self._directive_at = [position for position, _, _ in deltas]
        self._depth = []
        running = 0
        for _, delta, _ in deltas:
            running += delta
            self._depth.append(running)
        # 最外层的函数体（含 lambda）；其中定义的类是局部类，不进入索引。
        # Outermost function bodies (lambdas included); classes defined in them are local
        # and stay out of the index.
        self._bodies = []
        for start, end in sorted(
            (node.span.start, node.span.end) for node in self.nodes.get("compound_statement", [])
        ):
            if not self._bodies or start >= self._bodies[-1][1]:
                self._bodies.append((start, end))
        self._body_starts = [start for start, _ in self._bodies]

    def tokens_in(self, start: int, end: int) -> list[Token]:
        """字节范围 [start, end) 内开始的 token。
        The tokens starting in the byte range [start, end).
        """
        lo = bisect.bisect_left(self._starts, start)
        hi = bisect.bisect_left(self._starts, end)
        return self.tokens[lo:hi]

    def index_of(self, token: Token) -> int:
        """token 在 self.tokens 中的下标。
        The index of token in self.tokens.
        """
        return bisect.bisect_left(self._starts, token.span.start)

    def conditional_depth(self, token: Token) -> int:
        """token 处未闭合的 #if/#ifdef/#ifndef 层数，不计 include guard。
        The open #if/#ifdef/#ifndef levels at token, not counting an include guard.
        """
        i = bisect.bisect_right(self._directive_at, token.span.start)
        return self._depth[i - 1] if i else 0

    def in_function_body(self, byte_position: int) -> bool:
        """字节位置是否在函数体或 lambda 体内。
        Whether a byte position lies in a function or lambda body.
        """
        i = bisect.bisect_right(self._body_starts, byte_position)
        return bool(i) and self._bodies[i - 1][0] < byte_position < self._bodies[i - 1][1]


@functools.lru_cache(maxsize=256)
def _parsed_header(source_name: str | None, text: str) -> _Header:
    """同一文本只解析一次的 _Header；类型索引和模块接口共用。
    A _Header parsed once per text, shared by the type index and Module interfaces.
    """
    return _Header(Path(source_name) if source_name else None, text)


def _enclosing_scopes(header: _Header, byte_position: int) -> tuple[str, ...]:
    """字节位置所在的命名空间和类名，由外到内。
    The enclosing namespace and class names of a byte position, outermost first.
    """
    if header.scopes is None:
        header.scopes = []
        for kind in _SCOPES:
            for node in header.nodes.get(kind, []):
                body = node.child_by_field("body")
                if body is None:
                    continue
                name = node.child_by_field("name")
                header.scopes.append(
                    (body.span.start, body.span.end, name.text.strip() if name is not None else "")
                )
        header.scopes.sort()
    return tuple(name for start, end, name in header.scopes if start < byte_position < end)


def class_scope_names(
    source: str, name: str, source_name: str | None = None
) -> tuple[dict[str, str], dict[str, str]]:
    """全局类 name 的作用域中可见的名字及其访问权限，以及成员别名指向的写法。
    The names visible in the scope of the global class name with their access, and the
    spellings its member aliases stand for.

    可见的名字是成员类型、别名、非限定枚举的枚举值和静态成员，与类型索引读取类体的方式相同。
    The visible names are member types, aliases, enumerators of unscoped enums and static
    members, read the same way the type index reads class bodies.

    Raises:
        ValueError: 源码中没有这个全局类的定义。
            The source defines no global class of that name.
    """
    header = _parsed_header(source_name, source)
    for kind in ("class_specifier", "struct_specifier"):
        for node in header.nodes.get(kind, []):
            view = CppClassView(node)
            if view.name != name or view.body is None:
                continue
            if header.in_function_body(node.span.start) or _enclosing_scopes(
                header, node.span.start
            ):
                continue
            tokens = header.tokens_in(view.body.span.start, view.body.span.end)
            layout = _scan_body(
                header, tokens, "public" if kind == "struct_specifier" else "private"
            )
            aliases = {
                alias: header.text[start:end].strip()
                for alias, (start, end) in layout.aliases.items()
            }
            return dict(layout.names), aliases
    raise ValueError(f"No definition of class {name}")


class _Layout:
    """一个类体或命名空间体中的声明：数据成员、可见的名字，以及判断聚合体所需的信息。
    The declarations of one class or namespace body: data members, visible names and the
    facts that decide whether a class is an aggregate.
    """

    def __init__(self) -> None:
        self.fields: list[tuple[str, str, str]] = []  # (name, type spelling, access)
        self.field_defaults: dict[str, str] = {}  # default member initializer text
        self.conditional_fields: list[str] = []
        self.has_union = False
        self.has_virtual = False
        # 可以不加对象直接用名字引用的声明及其访问权限：类型、别名、枚举值、静态成员；
        # 命名空间中还有变量和函数。
        # Declarations referable by name without an object, with their access: types,
        # aliases, enumerators and static members; in a namespace also variables and
        # functions.
        self.names: dict[str, str] = {}
        self.aliases: dict[str, tuple[int, int]] = {}
        self.monitor: str | None = None  # 'public' / 'conditional' / None


def _scan_body(
    header: _Header, items: Sequence[Token], default_access: str, namespace_scope: bool = False
) -> _Layout:
    """扫描一个类体或命名空间体的 token（含两侧花括号）。
    Scan the tokens of one class or namespace body, braces included.

    Args:
        namespace_scope: 扫描的是命名空间体；其中的变量和函数也是可见的名字，嵌套的命名空间
            和 extern "C" 块跳过（嵌套命名空间另行扫描）。
            The body is a namespace body: its variables and functions are visible names too,
            and nested namespaces and extern "C" blocks are skipped (nested namespaces are
            scanned on their own).
    """
    layout = _Layout()
    access = default_access
    i, end = 1, len(items) - 1
    while i < end:
        start = i
        first = items[i]
        if (
            first.text in ("public", "protected", "private")
            and i + 1 < end
            and items[i + 1].text == ":"
        ):
            access = first.text
            i += 2
            continue
        if first.text == ";":
            i += 1
            continue
        # 收集一个声明，直到顶层的 ';' 或函数体结束。
        # Collect one declaration up to its top-level ';' (or a function body).
        j = i
        body_close = None
        skip = False
        while j < end and items[j].text != ";":
            if items[j].text == "{":
                close = close_token(items, j)
                previous = items[j - 1] if j > i else None
                if namespace_scope and (
                    any(t.text == "namespace" for t in items[i:j])
                    or (
                        first.text == "extern"
                        and previous is not None
                        and previous.kind == "literal"
                    )
                ):
                    body_close, skip = close, True
                    break
                # 函数声明中，紧跟成员名的花括号是构造函数初始化，其他花括号开始函数体。
                # In a function declaration, a brace right after a member name is a
                # constructor initializer; any other brace opens the body.
                if (
                    previous is not None
                    and _is_function(items[i:j])
                    and (
                        previous.kind != "identifier"
                        or previous.text in ("const", "override", "final", "noexcept")
                        or any(t.text == "->" for t in items[i:j])
                    )
                    and previous.text != ">"
                ):
                    body_close = close  # function body: the member ends here
                    break
                j = close + 1
                continue
            if items[j].text in ("(", "[", "<") and j + 1 < end:
                if items[j].text == "<" and not _template_open(items, i, j):
                    j += 1
                    continue
                j = close_token(items, j) + 1
                continue
            j += 1
        member = items[i : (body_close + 1 if body_close is not None else j)]
        i = (body_close + 1) if body_close is not None else j + 1
        if member and not skip:
            _classify(header, member, access, layout, namespace_scope)
        if start == i:
            i += 1
    if namespace_scope:
        for name, _type, field_access in layout.fields:
            layout.names.setdefault(name, field_access)
    return layout


def _template_open(items: Sequence[Token], start: int, index: int) -> bool:
    """index 处的 '<' 是否开始模板实参（而不是比较运算）。
    Whether the '<' at index opens template arguments rather than a comparison.
    """
    previous = items[index - 1] if index > start else None
    return previous is not None and (previous.kind == "identifier" or previous.text == "template")


def _enumerators(tokens: Sequence[Token]) -> list[str]:
    """枚举体 token（含两侧花括号）中的枚举值名。
    The enumerator names in the tokens of an enum body, braces included.
    """
    names, expect, k = [], True, 1
    while k < len(tokens) - 1:
        token = tokens[k]
        if token.text in ("(", "{", "[", "<"):
            k = close_token(tokens, k) + 1
            continue
        if expect and token.kind == "identifier":
            names.append(token.text)
        expect = token.text == ","
        k += 1
    return names


def _declared_name(member: Sequence[Token]) -> str | None:
    """声明所引入的名字：第一个顶层 '('、'='、'{'、'['、':' 或 ';' 之前的最后一个标识符；
    带限定的名字（如 Other::Run）不是新名字，返回 None。
    The name a declaration introduces: the last identifier before the first top-level
    '(', '=', '{', '[', ':' or ';'; a qualified name (such as Other::Run) introduces
    nothing new and gives None.
    """
    found = None
    k = 0
    while k < len(member):
        token = member[k]
        if token.text in ("(", "=", "{", ";", ":") or (
            token.text == "[" and not (k + 1 < len(member) and member[k + 1].text == "[")
        ):
            break
        if token.text == "<" and k and member[k - 1].kind == "identifier":
            k = close_token(member, k) + 1
            continue
        if token.text == "[":  # [[attribute]]
            k = close_token(member, k) + 1
            continue
        if token.text in ("alignas", "__attribute__") and k + 1 < len(member):
            k = close_token(member, k + 1) + 1
            continue
        if token.text == "operator":
            return None
        if token.kind == "identifier" and token.text not in _SPECIFIERS:
            found = None if k and member[k - 1].text == "::" else token.text
        k += 1
    return found


def _classify(
    header: _Header,
    member: Sequence[Token],
    access: str,
    layout: _Layout,
    namespace_scope: bool = False,
) -> None:
    """按一个声明更新 layout：数据成员、可见的名字、别名或 OnMonitor。
    Update layout from one declaration: a data member, a visible name, an alias or
    OnMonitor.
    """
    texts = [t.text for t in member]
    first = texts[0]
    if first == "template":
        # 模板声明按模板头之后的声明登记。
        # A template declaration is registered by the declaration after its header.
        if len(member) > 2 and member[1].text == "<":
            close = close_token(member, 1)
            if close + 1 < len(member):
                _classify(header, member[close + 1 :], access, layout, namespace_scope)
        return
    if first == "using":
        if len(texts) >= 4 and texts[2] == "=" and member[1].kind == "identifier":
            layout.names[texts[1]] = access
            layout.aliases[texts[1]] = (member[2].end, member[-1].end)
        elif "OnMonitor" in texts and access == "public":
            layout.monitor = "conditional" if header.conditional_depth(member[0]) else "public"
        return
    if first == "typedef":
        # typedef struct {...} Name; 由 TypeIndex._parse 登记为类。
        # typedef struct {...} Name; is registered as a class by TypeIndex._parse.
        if member[-1].kind == "identifier":
            layout.names[texts[-1]] = access
        if texts[1:2] == ["enum"] and "{" in texts:
            opening = texts.index("{")
            for enumerator in _enumerators(member[opening : close_token(member, opening) + 1]):
                layout.names[enumerator] = access
        return
    if first in _SKIP_LEADING:
        return
    if first in _CLASS_KEYS:
        k = 1
        if first == "enum" and k < len(texts) and texts[k] in ("class", "struct"):
            k += 1
        name = texts[k] if k < len(texts) and member[k].kind == "identifier" else None
        opening = next((m for m, t in enumerate(texts) if t == "{"), None)
        if first == "union":
            layout.has_union = True
        if name is not None and opening is not None:
            layout.names[name] = access
        if opening is None:
            return  # forward declaration or elaborated member type handled below
        close = close_token(member, opening)
        if first == "enum" and texts[1] not in ("class", "struct"):
            # 非限定枚举的枚举值在外层作用域可见。
            # The enumerators of an unscoped enum are visible in the enclosing scope.
            for enumerator in _enumerators(member[opening : close + 1]):
                layout.names[enumerator] = access
        rest = member[close + 1 :]
        declarators = [t for t in rest if t.text != ";"]
        if declarators and first != "enum":
            # `struct T {...} member;` 声明一个嵌套类型的成员。
            # `struct T {...} member;` declares a member of the nested type.
            _record_fields(header, declarators, name or "", access, layout)
        elif declarators and first == "enum":
            _record_fields(header, declarators, name or "int", access, layout)
        return
    if "virtual" in texts:
        layout.has_virtual = True
    # 前导说明符决定是否为静态成员；函数在初始化之前有顶层的 '('。
    # Leading specifiers decide static members; functions have a top-level '(' before
    # any initializer.
    leading = set()
    for t in texts:
        if t in _SPECIFIERS:
            leading.add(t)
        elif t in ("[", "alignas", "__attribute__"):
            continue
        else:
            break
    function = _is_function(member)
    if "static" in leading or (namespace_scope and function):
        name = _declared_name(member)
        if name is not None:
            layout.names[name] = access
    if "static" in leading or function:
        if "OnMonitor" in texts and access == "public" and function:
            layout.monitor = "conditional" if header.conditional_depth(member[0]) else "public"
        return
    _record_fields(header, member, None, access, layout)


def _is_function(member: Sequence[Token]) -> bool:
    """成员声明是否声明函数：在 '='、'{' 或 ':' 之前出现顶层的 '('。
    Whether a member declaration declares a function: a top-level '(' before any '=',
    '{' or ':'.
    """
    k = 0
    while k < len(member):
        text = member[k].text
        if text in ("=", "{", ":"):
            return False
        if text == "[" and k + 1 < len(member) and member[k + 1].text == "[":
            k = close_token(member, k) + 1
            continue
        if (
            text in ("alignas", "__attribute__", "decltype")
            and k + 1 < len(member)
            and member[k + 1].text == "("
        ):
            k = close_token(member, k + 1) + 1
            continue
        if text == "<":
            k = close_token(member, k) + 1
            continue
        if text == "(":
            return True
        k += 1
    return False


def _record_fields(
    header: _Header,
    member: Sequence[Token],
    forced_type: str | None,
    access: str,
    layout: _Layout,
) -> None:
    """记录一个数据成员声明中的每个字段、类型写法和默认初始化。
    Record every field of one data-member declaration with its type spelling and
    default member initializer.

    Args:
        forced_type: 字段类型已知时（如 struct T {...} member;）的类型写法，否则为 None。
            The type spelling when it is already known (as in struct T {...} member;),
            else None.
    """
    parts, start, k = [], 0, 0
    while k < len(member):
        text = member[k].text
        if text in ("(", "{", "[", "<"):
            if text == "<" and not (k and member[k - 1].kind == "identifier"):
                k += 1
                continue
            k = close_token(member, k) + 1
            continue
        if text == ",":
            parts.append(member[start:k])
            start = k + 1
        if text == ";":
            parts.append(member[start:k])
            start = k + 1
        k += 1
    if start < len(member):
        parts.append(member[start:])
    base_type = forced_type
    conditional = bool(member) and header.conditional_depth(member[0]) > 0
    for part in parts:
        if not part:
            continue
        name_index = None
        m = 0
        while m < len(part):
            token = part[m]
            if m and token.text in ("{", "=", "[", ":"):
                break
            if (
                token.text in ("alignas", "__attribute__")
                and m + 1 < len(part)
                and part[m + 1].text == "("
            ):
                m = close_token(part, m + 1) + 1
                continue
            if token.text == "[" and m + 1 < len(part) and part[m + 1].text == "[":
                m = close_token(part, m) + 1
                continue
            if token.text == "<":
                m = close_token(part, m) + 1
                continue
            if token.kind == "identifier" and token.text not in _SPECIFIERS:
                name_index = m
            m += 1
        if name_index is None:
            continue
        if base_type is None:
            type_tokens = list(part[:name_index])
            spelled = []
            n = 0
            while n < len(type_tokens):
                token = type_tokens[n]
                if (
                    token.text in ("alignas", "__attribute__")
                    and n + 1 < len(type_tokens)
                    and type_tokens[n + 1].text == "("
                ):
                    n = close_token(type_tokens, n + 1) + 1
                    continue
                if (
                    token.text == "["
                    and n + 1 < len(type_tokens)
                    and type_tokens[n + 1].text == "["
                ):
                    n = close_token(type_tokens, n) + 1
                    continue
                if token.text == "mutable":
                    n += 1
                    continue
                spelled.append(token)
                n += 1
            base_type = header.text[spelled[0].start : spelled[-1].end].strip() if spelled else ""
        name = part[name_index].text
        layout.fields.append((name, base_type, access))
        rest = part[name_index + 1 :]
        if rest and rest[0].text == ":":  # bit-field width, then an optional initializer
            rest = next((rest[k:] for k in range(len(rest)) if rest[k].text in ("=", "{")), [])
        if rest and rest[0].text == "=" and len(rest) > 1:
            layout.field_defaults[name] = header.text[rest[1].start : rest[-1].end].strip()
        elif rest and rest[0].text == "{":
            layout.field_defaults[name] = header.text[rest[0].start : rest[-1].end].strip()
        if conditional:
            layout.conditional_fields.append(name)


class ClassEntry:
    """索引中的一个 class/struct 定义。
    One located class/struct definition.
    """

    def __init__(
        self,
        header: _Header,
        path: tuple[str, ...],
        kind: str,
        body_range: tuple[int, int],
        view: CppClassView | None = None,
        head_range: tuple[int, int] | None = None,
        template_parameters: list[str] | None = None,
    ) -> None:
        self.header_info = header
        self.header = header.path
        self.path = path
        self.kind = kind
        self.view = view
        self._body_range = body_range  # token indices of '{' and '}'
        self._head_range = head_range
        self.template_parameters = template_parameters or []
        self._layout: _Layout | None = None

    @property
    def qualified(self) -> str:
        """带命名空间和外层类的全名。
        The name qualified with its namespaces and enclosing classes.
        """
        return "::".join(self.path)

    @property
    def default_access(self) -> str:
        """未写访问说明符时成员的访问权限。
        The access of members before any access specifier.
        """
        return "public" if self.kind == "struct" else "private"

    def body_tokens(self) -> list[Token]:
        """类体的 token，含两侧花括号。
        The tokens of the class body, braces included.
        """
        start, end = self._body_range
        return self.header_info.tokens[start : end + 1]

    def _head_tokens(self) -> list[Token]:
        """类名到类体之前的 token（含基类列表）。
        The tokens from the class key to the body, base clause included.
        """
        if self._head_range is None:
            return []
        start, end = self._head_range
        return self.header_info.tokens[start:end]

    def layout(self) -> _Layout:
        """类体扫描结果，首次调用时扫描。
        The scanned class body, scanned on first use.
        """
        if self._layout is None:
            self._layout = _scan_body(self.header_info, self.body_tokens(), self.default_access)
        return self._layout

    def has_base(self) -> bool:
        """是否有基类。
        Whether the class has base classes.
        """
        return any(t.text == ":" for t in self._head_tokens())

    def base_spellings(self) -> list[tuple[str, str]]:
        """每个直接基类的 (访问权限, 类型写法)。
        The (access, type spelling) of each direct base class.
        """
        head = self._head_tokens()
        colon = next(
            (
                i
                for i, t in enumerate(head)
                if t.text == ":"
                and not (i + 1 < len(head) and head[i + 1].text == ":")
                and not (i and head[i - 1].text == ":")
            ),
            None,
        )
        if colon is None:
            return []
        result, start, i = [], colon + 1, colon + 1
        while i <= len(head):
            if i < len(head) and head[i].text == "<":
                i = close_token(head, i) + 1
                continue
            if i == len(head) or head[i].text == ",":
                words = [t for t in head[start:i] if t.text != "virtual"]
                access = self.default_access
                if words and words[0].text in ("public", "protected", "private"):
                    access = words[0].text
                    words = words[1:]
                if words:
                    result.append(
                        (access, self.header_info.text[words[0].start : words[-1].end].strip())
                    )
                start = i + 1
            i += 1
        return result

    def constructors(self) -> list[list[dict]]:
        """可调用的公有构造函数，每个为参数描述的列表。
        The callable public constructors, each a list of parameter descriptions.
        """
        if self.view is None:
            return []
        from xrobot.constructor_model import parameter

        return [
            [parameter(p.text) for p in ctor.parameters if p.text.strip() not in ("", "void")]
            for ctor in self.view.constructors(public_only=True, callable_only=True)
        ]

    def declares_constructor(self) -> bool:
        """是否声明了任何构造函数。
        Whether the class declares any constructor.
        """
        return self.view is not None and bool(self.view.constructors())

    def fields(self) -> list[tuple[str, str, str]]:
        """非静态数据成员的 (名字, 类型写法, 访问权限)，按声明顺序。
        The (name, type spelling, access) of the non-static data members in declaration
        order.
        """
        return list(self.layout().fields)

    def is_aggregate(self) -> bool:
        """是否为聚合体：没有构造函数、基类和虚函数，数据成员都是公有的。
        Whether the class is an aggregate: no constructors, bases or virtual functions,
        and only public data members.
        """
        layout = self.layout()
        if self.declares_constructor() or self.has_base() or layout.has_virtual:
            return False
        return all(access == "public" for _, _, access in layout.fields)

    def mapping_problem(self) -> str | None:
        """YAML 映射不能对照此类型检查的原因；可以检查时为 None。
        Why a YAML mapping cannot be checked against this type, or None when it can.
        """
        layout = self.layout()
        if layout.has_union:
            return f"{self.qualified} contains a union"
        if layout.conditional_fields:
            return f"{self.qualified} declares fields under #if ({', '.join(layout.conditional_fields)})"
        if not self.declares_constructor() and (self.has_base() or layout.has_virtual):
            return f"{self.qualified} has base classes or virtual functions and no constructor"
        return None

    def monitor(self) -> str | None:
        """类自身声明的 OnMonitor：'public'、'conditional'（在 #if 中）或 None。
        The OnMonitor the class itself declares: 'public', 'conditional' (under #if) or
        None.
        """
        return self.layout().monitor


class TypeIndex:
    """已加载模块头文件中 class/struct 定义的按需索引。
    On-demand index of the class/struct definitions in loaded Module headers.
    """

    def __init__(self, headers: Iterable[Path], labels: dict[Path, str] | None = None) -> None:
        """建立索引；头文件在查询时才解析。
        Create the index; headers are parsed when a query needs them.

        Args:
            headers: 要索引的头文件。
                The headers to index.
            labels: 报错时头文件的名字，缺省为完整路径。
                The names of headers in error messages; the full path by default.
        """
        self.headers = sorted({Path(h).resolve() for h in headers})
        self._labels = {Path(k).resolve(): v for k, v in (labels or {}).items()}
        self._texts: dict[Path, str] = {}
        self._parsed: dict[Path, _Header] = {}
        self._entries: dict[tuple[str, ...], list[ClassEntry]] = {}
        self._aliases: dict[tuple[str, ...], tuple[_Header, int, int]] = {}
        self._namespace_names: dict[tuple[str, ...], set] = {}
        self._ensured: set = set()
        self._resolved: dict[tuple[str, tuple[str, ...]], ClassEntry | None] = {}
        self._class_candidates: set[str] | None = None

    @classmethod
    def for_modules(cls, modules: dict) -> TypeIndex:
        """一组模块全部头文件的索引；报错时头文件写作 owner/Repo/模块内路径。
        The index of every header of a set of Modules; error messages name a header as
        owner/Repo/<path in the Module>.
        """
        labels = {}
        for module in modules.values():
            folder = Path(module["path"])
            for header in _headers_of(module):
                labels[header] = f"{module['id']}/{header.relative_to(folder).as_posix()}"
        return cls(list(labels), labels)

    def _text(self, path: Path) -> str:
        """头文件文本，只读取一次。
        The text of a header, read once.
        """
        if path not in self._texts:
            self._texts[path] = path.read_text(encoding="utf-8", errors="surrogateescape")
        return self._texts[path]

    def _label(self, path: Path) -> str:
        """报错时头文件的名字。
        The name of a header in error messages.
        """
        return self._labels.get(path, path.as_posix())

    def is_global_class(self, name: str) -> bool:
        """name 是否是已加载头文件在全局作用域定义的 class/struct。
        Whether name is a class/struct defined at global scope in the loaded headers.

        先扫描一遍文本，收集可能是类定义的名字；name 在其中时再解析相关头文件确认作用域。
        One text scan collects the names that may be class definitions; only for such a
        name are the headers that mention it parsed to confirm the scope.
        """
        if self._class_candidates is None:
            pattern = re.compile(
                rf"\b(?:class|struct)\s+{_ATTRIBUTES}([A-Za-z_]\w*)|\}}\s*([A-Za-z_]\w*)\s*;"
            )
            self._class_candidates = {
                match.group(1) or match.group(2)
                for path in self.headers
                for match in pattern.finditer(self._text(path))
            }
        if name not in self._class_candidates:
            return False
        self._ensure(name)
        return bool(self._entries.get((name,)))

    def _ensure(self, name: str) -> None:
        """解析文本中出现 name 的全部头文件。
        Parse every header whose text mentions name.
        """
        if name in self._ensured:
            return
        self._ensured.add(name)
        pattern = re.compile(rf"\b{re.escape(name)}\b")
        for path in self.headers:
            if path not in self._parsed and pattern.search(self._text(path)):
                self._parse(path)

    def _add(self, entry: ClassEntry) -> None:
        """登记一个类及其成员别名。
        Register a class and its member aliases.
        """
        self._entries.setdefault(entry.path, []).append(entry)
        for alias, (start, end) in entry.layout().aliases.items():
            self._aliases.setdefault(entry.path + (alias,), (entry.header_info, start, end))

    def _parse(self, path: Path) -> None:
        """解析一个头文件，登记其中命名空间和类作用域中的类、别名和类型名。
        Parse one header and register the classes, aliases and type names of its
        namespace and class scopes.
        """
        header = _parsed_header(str(path), self._text(path))
        self._parsed[path] = header
        classes = header.nodes.get("class_specifier", []) + header.nodes.get("struct_specifier", [])
        for view in (CppClassView(node) for node in classes):
            if view.body is None or not view.name:
                continue
            node = view.node
            if header.in_function_body(node.span.start):
                continue
            scope = _enclosing_scopes(header, node.span.start)
            body_tokens = header.tokens_in(view.body.span.start, view.body.span.end)
            if not body_tokens:
                continue
            open_index = header.index_of(body_tokens[0])
            close_index = header.index_of(body_tokens[-1])
            head_tokens = header.tokens_in(node.span.start, view.body.span.start)
            head_range = (header.index_of(head_tokens[0]), open_index) if head_tokens else None
            parameters = [p.name for p in view.template_parameters if getattr(p, "name", None)]
            kind = "struct" if node.kind == "struct_specifier" else "class"
            self._add(
                ClassEntry(
                    header,
                    scope + (view.name,),
                    kind,
                    (open_index, close_index),
                    view=view,
                    head_range=head_range,
                    template_parameters=parameters,
                )
            )
        # 命名空间作用域的 typedef struct {...} Name;（xr-syntax 不把它建模为类）。
        # typedef struct {...} Name; at namespace scope (xr-syntax does not model it).
        items = header.tokens
        for k, token in enumerate(items[:-2]):
            if token.text != "typedef" or items[k + 1].text not in ("struct", "class"):
                continue
            if header.in_function_body(token.span.start):
                continue
            opening = next(
                (m for m in range(k + 2, min(k + 6, len(items))) if items[m].text == "{"), None
            )
            if opening is None:
                continue
            closing = close_token(items, opening)
            names = [t.text for t in items[closing + 1 : closing + 4] if t.kind == "identifier"]
            if not names:
                continue
            scope = _enclosing_scopes(header, token.span.start)
            if scope + (names[0],) not in self._entries:
                self._add(ClassEntry(header, scope + (names[0],), "struct", (opening, closing)))
        for namespace in header.nodes.get("namespace_definition", []):
            name = namespace.child_by_field("name")
            body = namespace.child_by_field("body")
            if name is None or body is None:
                continue
            scope = _enclosing_scopes(header, namespace.span.start) + (name.text.strip(),)
            tokens = header.tokens_in(body.span.start, body.span.end)
            layout = _scan_body(header, tokens, "public", namespace_scope=True)
            self._namespace_names.setdefault(scope, set()).update(layout.names)
            for alias, (start, end) in layout.aliases.items():
                self._aliases.setdefault(scope + (alias,), (header, start, end))

    # -- lookup -------------------------------------------------------------

    def _class_at(self, path: tuple[str, ...]) -> ClassEntry | None:
        """路径处定义的类。
        The class defined at path.

        Raises:
            ValueError: 该类型在多个头文件中定义。
                The type is defined in several headers.
        """
        entries = self._entries.get(path)
        if not entries:
            return None
        headers = sorted({self._label(e.header) for e in entries})
        if len(headers) > 1:
            raise ValueError(
                f"Type {'::'.join(path)} is defined in several Module headers: {', '.join(headers)}"
            )
        return entries[0]

    def resolve(
        self, spelling: str, scope: tuple[str, ...] = (), _depth: int = 0
    ) -> ClassEntry | None:
        """类型写法指向的类，从 scope 开始向外查找。
        The class a type spelling names, searching outward from scope.

        没有找到，或名字可能指向索引之外的东西（外层类模板的模板参数、不在已加载头文件中的
        基类成员）时返回 None。
        None when the name is not found or may denote something outside the index (a
        template parameter of an enclosing class template, a member of a base class
        outside the loaded headers).
        """
        key = (spelling, tuple(scope))
        if _depth == 0 and key in self._resolved:
            return self._resolved[key]
        result = self._resolve(spelling, tuple(scope), _depth)
        if _depth == 0:
            self._resolved[key] = result
        return result

    def _resolve(self, spelling: str, scope: tuple[str, ...], _depth: int) -> ClassEntry | None:
        """resolve 的实现，不带缓存。
        The uncached implementation of resolve.
        """
        names = _strip_type(spelling)
        if not names or _depth > 8:
            return None
        for name in {names[0], names[-1]}:
            self._ensure(name)
        for i in range(len(scope), -1, -1):
            path = tuple(scope[:i])
            owner = self._class_at(path) if path else None
            if owner is not None and names[0] in owner.template_parameters:
                return None
            found, certain = self._lookup_in(path, names, owner, _depth)
            if found is not None:
                return found
            if not certain:
                return None
        return None

    def _lookup_in(
        self,
        path: tuple[str, ...],
        names: tuple[str, ...],
        owner: ClassEntry | None,
        depth: int,
    ) -> tuple[ClassEntry | None, bool]:
        """直接在 path 中查找 names；path 是类时也查找其基类。
        Look up names directly in path and, when path is a class, in its bases.

        Returns:
            (找到的类或 None, 结果是否确定)；基类无法定位时不确定。
            (the class found or None, whether the answer is certain); it is not certain
            when a base class cannot be located.
        """
        target = path + names
        entry = self._class_at(target)
        if entry is not None:
            return entry, True
        alias = self._aliases.get(target)
        if alias:
            header, start, end = alias
            return self.resolve(header.text[start:end].strip(), target[:-1], depth + 1), True
        if owner is None:
            return None, True
        for _access, base in owner.base_spellings():
            parent = self.resolve(base, path[:-1], depth + 1)
            if parent is None:
                return None, False
            found, certain = self._lookup_in(parent.path, names, parent, depth + 1)
            if found is not None or not certain:
                return found, certain
        return None, True

    def qualify_in(self, spelling: str, entry: ClassEntry, entry_spelled: str) -> str:
        """给 spelling 中在 entry 所在类/命名空间链上声明的名字加上限定。
        Qualify the names in spelling that are declared in entry's class/namespace chain.

        Args:
            entry_spelled: 生成代码中 entry 的写法，可带模板实参（如 Outer<T>::Param）；
                外层类模板的模板参数替换为这些实参。
                How generated code names entry; it may carry template arguments (e.g.
                Outer<T>::Param), which replace the template parameters of the enclosing
                class templates.
        """
        scopes = []  # (declared names, spelled prefix), innermost first
        replacements: dict[str, str] = {}
        spelled, path = entry_spelled, entry.path
        while path:
            outer, args = _split_last(spelled) if spelled else ("", None)
            owner = self._class_at(path)
            if owner is not None:
                scopes.append((set(owner.layout().names), spelled))
                if owner.template_parameters and args is not None:
                    for name, value in zip(owner.template_parameters, args, strict=False):
                        replacements.setdefault(name, value.strip())
            else:
                scopes.append((self._namespace_names.get(path, set()), "::".join(path)))
            path = path[:-1]
            spelled = outer or "::".join(path)
        spelling = _replace_identifiers(spelling, replacements)
        items = code_tokens(spelling)
        edits = []
        for i, token in enumerate(items):
            if token.kind != "identifier":
                continue
            if i and items[i - 1].text in ("::", ".", "->"):
                continue
            for names, prefix in scopes:
                if token.text in names and prefix:
                    edits.append((token.start, token.end, prefix + "::" + token.text))
                    break
        for start, end, text in reversed(edits):
            spelling = spelling[:start] + text + spelling[end:]
        return spelling

    def provides_monitor(self, entry: ClassEntry, _depth: int = 0) -> bool | None:
        """entry 或其公有基类是否声明了公有的 OnMonitor。
        Whether entry or a public base declares a public OnMonitor.

        Returns:
            公有基类无法定位时为 None（LibXR 基类不提供 OnMonitor，不计入）。
            None when a public base cannot be located (LibXR bases never provide
            OnMonitor and are skipped).

        Raises:
            ValueError: OnMonitor 声明在 #if 中。
                OnMonitor is declared under #if.
        """
        state = entry.monitor()
        if state == "conditional":
            raise ValueError(
                f"{entry.qualified} declares OnMonitor under #if; the generator cannot evaluate "
                "build options"
            )
        if state == "public":
            return True
        if _depth > 8:
            return None
        unknown = False
        for access, base in entry.base_spellings():
            if access != "public":
                continue
            if base.lstrip(":").startswith("LibXR::"):
                continue
            parent = self.resolve(base, entry.path[:-1])
            if parent is None:
                unknown = True
                continue
            found = self.provides_monitor(parent, _depth + 1)
            if found:
                return True
            if found is None:
                unknown = True
        return None if unknown else False


def _headers_of(module: dict) -> list[Path]:
    """一个模块的头文件：根目录下的 *.hpp，以及它们经 #include "..." 引入、位于模块目录内的头文件。
    The headers of one Module: the *.hpp in its root folder and the headers inside the
    Module folder that they bring in with #include "...".

    引号中的路径先相对包含它的文件所在目录查找，再相对模块根目录查找，与编译器相同。
    A quoted path is looked up next to the including file first and then in the Module
    root, as the compiler does.
    """
    folder = Path(module["path"])
    root = folder.resolve()
    pending = sorted(folder.glob("*.hpp"))
    seen: set[Path] = set()
    while pending:
        path = pending.pop()
        if path in seen:
            continue
        seen.add(path)
        text = path.read_text(encoding="utf-8", errors="surrogateescape")
        for name in _QUOTED_INCLUDE.findall(text):
            for base in (path.parent, folder):
                candidate = Path(os.path.normpath(base / name))
                if candidate.is_file():
                    if candidate.resolve().is_relative_to(root):
                        pending.append(candidate)
                    break
    return sorted(seen)


def module_headers(modules: dict) -> list[Path]:
    """生成器对一组模块可能读取的全部头文件。
    Every header the generator may read for a set of Modules.
    """
    headers = []
    for module in modules.values():
        headers.extend(_headers_of(module))
    return headers
