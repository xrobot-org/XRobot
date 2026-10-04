"""XRobot 对 xr-syntax 的薄适配层。
Thin XRobot adapter over xr-syntax.

这里只保留 XRobot 需要的源码操作：代码 token、括号配对、参数列表切分、预处理条件层数，以及
模块构造接口的提取。C++ 解析全部由 xr-syntax 负责。
It keeps only the source operations XRobot needs: code tokens, delimiter matching,
argument-list splitting, preprocessor conditional depth and the extraction of a Module's
constructor interface. xr-syntax does all C++ parsing.
"""

from __future__ import annotations

import bisect
import functools
import re
from collections.abc import Sequence

from xr_syntax.cpp import (
    CppDocument,
    CppLexicalToken,
    matching_delimiter,
    split_source_list,
)
from xr_syntax.cpp import (
    code_tokens as _code_tokens,
)
from xr_syntax.i18n import tr

Token = CppLexicalToken


@functools.lru_cache(maxsize=65536)
def _cached_tokens(source: str) -> tuple[CppLexicalToken, ...]:
    """source 的代码 token，按文本缓存。
    The code tokens of source, cached per text.
    """
    return tuple(_code_tokens(source))


def code_tokens(source: str) -> list[CppLexicalToken]:
    """source 的代码 token（不含注释和空白），每次返回新的列表，调用方可以修改。
    The code tokens of source (no comments or whitespace), as a new list each time so
    callers may modify it.

    同一段文本的切分结果有缓存：生成时同样的类型写法和头文件会被反复读取。
    Tokenizing a given text is cached: generation reads the same type spellings and
    headers many times.
    """
    return list(_cached_tokens(source))


@functools.lru_cache(maxsize=256)
def parse_document(source: str, source_name: str | None = None) -> CppDocument:
    """解析 C++ 源码；同样的 (文本, 名字) 只解析一次，文档不可变。
    Parse C++ source once per (text, name); documents are immutable.
    """
    return CppDocument.parse(source, source_name=source_name)


def close_token(items: Sequence[CppLexicalToken], start: int) -> int:
    """与 items[start] 这个左括号配对的右括号的下标。
    The index of the closing delimiter that matches the opening one at items[start].
    """
    return matching_delimiter(items, start)


def split_arguments(text: str) -> list[str]:
    """按顶层逗号切分 C++ 参数或声明列表；模板尖括号内的逗号不切。
    Split a C++ argument or declaration list on top-level commas; commas inside template
    angle brackets do not split.
    """
    return list(split_source_list(text, template_angles=True))


_DIRECTIVES = frozenset({"preproc_if", "preproc_ifdef", "preproc_call"})


@functools.lru_cache(maxsize=256)
def _directive_depths(document: CppDocument) -> tuple[list[int], list[int]]:
    """文档中 #if/#ifdef/#ifndef/#endif 的位置，以及每一处之后的嵌套层数。
    The positions of the #if/#ifdef/#ifndef/#endif directives of the document, and the
    nesting depth after each.
    """
    deltas = []
    for node in document.root.descendants(kinds=_DIRECTIVES):
        if node.kind in ("preproc_if", "preproc_ifdef"):
            deltas.append((node.span.start, 1))
        elif node.kind == "preproc_call":
            significant = [child for child in node.syntax_children if not child.is_trivia]
            if len(significant) > 1 and significant[1].text == "endif":
                deltas.append((node.span.start, -1))
    deltas.sort()
    positions = [position for position, _ in deltas]
    running, depths = 0, []
    for _, delta in deltas:
        running += delta
        depths.append(running)
    return positions, depths


def conditional_depth(document: CppDocument, start: int, end: int) -> int:
    """[start, end) 内打开而未关闭的 #if/#ifdef/#ifndef 层数。
    The number of #if/#ifdef/#ifndef opened but not closed within [start, end).

    指令由 xr-syntax 的预处理节点识别，每个文档只统计一次。
    The directives come from xr-syntax's preprocessor nodes and are counted once per
    document.
    """
    positions, depths = _directive_depths(document)
    before = bisect.bisect_left(positions, start)
    last = bisect.bisect_left(positions, end)
    base = depths[before - 1] if before else 0
    return (depths[last - 1] if last else 0) - base if last > before else 0


def extract_interface(source: str, name: str, source_name: str | None = None) -> dict:
    """头文件中全局类 name 的构造接口：模板参数声明和公有构造函数。
    The constructor interface of the global class name in a header: its template parameter
    declarations and public constructors.

    拷贝构造和移动构造不算接口：配置无法从另一个实例复制出模块。
    Copy and move constructors are not part of the interface: a configuration cannot copy
    a Module from another instance.

    Returns:
        含 name、template_declarations（模板参数声明，非模板类为空）和 constructors
        （每项含 declaration、parameters、line）的映射。
        A mapping with name, template_declarations (template parameter declarations, empty
        for a class that is not a template) and constructors (each with declaration,
        parameters and line).

    Raises:
        ValueError: 没有这个全局类或有多个，类是显式特化，没有可调用的公有构造函数，或
            构造函数在 #if 中。
            There is no such global class or several, the class is an explicit
            specialization, it has no callable public constructor, or a constructor is
            under #if.
    """
    document = parse_document(source, source_name)
    views = document.class_views(name)
    classes = [view for view in views if _is_global_class(view.node)]
    if not classes:
        if views:
            scope = _scope_of(views[0].node)
            raise ValueError(
                tr(
                    f"{name} is declared inside {scope}; a Module class must be declared at "
                    "global scope",
                    f"{name} 声明在{scope}中；模块类必须声明在全局作用域",
                )
            )
        raise ValueError(
            tr(
                f"No global class {name} is declared in this header",
                f"这个头文件中没有全局类 {name}",
            )
        )
    if len(classes) != 1:
        raise ValueError(
            tr(f"Multiple definitions of Module class {name}", f"模块类 {name} 有多个定义")
        )

    class_view = classes[0]
    parent = class_view.node.parent
    templated = parent is not None and parent.kind == "template_declaration"
    template_parameters = class_view.template_parameters
    if templated and not template_parameters:
        raise ValueError(
            tr(
                "Explicit Module template specialization is not supported",
                "不支持模块类的显式模板特化",
            )
        )

    constructors = [
        constructor
        for constructor in class_view.constructors(public_only=True, callable_only=True)
        if not copies_or_moves([p.text.strip() for p in constructor.parameters], name)
    ]
    if not constructors:
        raise ValueError(
            tr(
                f"No supported explicit public constructor for {name}",
                f"{name} 没有可用的显式公有构造函数",
            )
        )

    source_bytes = document.render_bytes()
    body = class_view.body
    result = []
    for constructor in constructors:
        if body is not None and conditional_depth(
            document, body.span.start, constructor.node.span.start
        ):
            raise ValueError(
                tr(
                    f"{name} constructor interface varies under #if",
                    f"{name} 的构造接口随 #if 变化",
                )
            )

        declarator = constructor.declarator
        result.append(
            {
                "declaration": (constructor.node.text if declarator is None else declarator.text),
                "parameters": [parameter.text.strip() for parameter in constructor.parameters],
                "line": source_bytes[: constructor.node.span.start].count(b"\n") + 1,
            }
        )
    return {
        "name": name,
        "template_declarations": [parameter.text.strip() for parameter in template_parameters],
        "constructors": result,
        "arities": constructor_arities(class_view, name),
    }


def parameter_arity(parameters: list[str]) -> tuple[int, int | None]:
    """参数表接受的实参个数：(没有默认值的参数个数, 最多个数；有可变参数时为 None)。
    The number of arguments a parameter list accepts: (parameters without a default, the most
    it takes; None with a pack).
    """
    declared = [p for p in parameters if p.strip() not in ("", "void")]
    packs = [p for p in declared if "..." in p]
    required = sum(
        1 for p in declared if p not in packs and not any(t.text == "=" for t in code_tokens(p))
    )
    if packs:
        return required, None
    return required, len(declared)


def constructor_arities(class_view, name: str) -> list[tuple[int, int | None]]:
    """类声明的每个构造函数（不论访问权限、是否可调用，拷贝和移动构造除外）接受的实参个数。
    The argument counts of every constructor the class declares, whatever its access or
    callability, copy and move constructors excepted.

    重载决议会考虑私有和已删除的构造函数，所以生成器看一个调用有几个候选时用这份列表。
    Overload resolution considers private and deleted constructors too, so the generator
    counts the candidates of a call from this list.
    """
    arities = []
    for constructor in class_view.constructors():
        parameters = [p.text.strip() for p in constructor.parameters]
        if not copies_or_moves(parameters, name):
            arities.append(parameter_arity(parameters))
    return arities


def viable_constructors(arities: list[tuple[int, int | None]], count: int) -> int:
    """能接受 count 个实参的构造函数个数。
    The number of constructors that take count arguments.
    """
    return sum(1 for low, high in arities if low <= count and (high is None or count <= high))


def copies_or_moves(parameters: list[str], name: str) -> bool:
    """参数表是否是 name 的拷贝构造或移动构造：唯一参数为 [const] name[<...>]& 或 &&。
    Whether a parameter list makes a copy or move constructor of name: its one parameter
    is [const] name[<...>]& or &&.
    """
    if len(parameters) != 1:
        return False
    tokens = code_tokens(parameters[0])
    texts = [t.text for t in tokens]
    if "=" in texts:
        tokens = tokens[: texts.index("=")]
    if len(tokens) >= 2 and tokens[-1].kind == "identifier" and tokens[-2].text in ("&", "&&"):
        tokens = tokens[:-1]
    texts = [t.text for t in tokens if t.text not in ("const", "volatile")]
    if len(texts) < 2 or texts[-1] not in ("&", "&&") or texts[0] != name:
        return False
    rest = texts[1:-1]
    return not rest or (rest[0] == "<" and rest[-1] == ">")


def _scope_of(node) -> str:
    """包含一个非全局类的作用域的说明，如 namespace team 或 class Outer。
    A description of the scope that holds a class that is not global, such as
    namespace team or class Outer.
    """
    parent = node.parent
    while parent is not None:
        if parent.kind == "namespace_definition":
            head = " ".join(parent.text.split("{", 1)[0].split())
            if head == "namespace":
                return tr("an anonymous namespace", "匿名命名空间")
            return tr(head, f" {head} ")
        if parent.kind in ("class_specifier", "struct_specifier"):
            match = re.match(r"\s*(class|struct)\s+(\w+)", parent.text)
            if match:
                return tr(
                    f"{match.group(1)} {match.group(2)}", f" {match.group(1)} {match.group(2)} "
                )
            return tr("another class", "另一个类")
        if parent.kind == "function_definition":
            return tr("a function body", "函数体")
        parent = parent.parent
    return tr("another declaration", "另一个声明")


def _is_global_class(node) -> bool:
    """class/struct 是否直接声明在翻译单元中（外面可以有一层 template）。
    Whether a class/struct is declared directly in the translation unit, optionally inside
    one template declaration.
    """
    parent = node.parent
    if parent is not None and parent.kind == "template_declaration":
        parent = parent.parent
    return parent is not None and parent.kind == "translation_unit"
