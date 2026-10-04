"""生成的 C++ 的排版：include 块、调用、花括号初始化器和宏，与 LibXR 的 .clang-format
（Google 风格、列宽 90、Allman 花括号、IncludeBlocks: Regroup）一致，不调用 clang-format。
Layout of the generated C++: the include block, calls, brace initializers and macros, matching
LibXR's .clang-format (Google based, column limit 90, Allman braces, IncludeBlocks: Regroup)
without running clang-format.

调用的实参放得下就写在一行，否则填满每一行并续行对齐到左括号之后；一个标识符都放不下时左括号
后换行，缩进 4 格。花括号初始化器放得下就写在一行，否则每项一行、每项带逗号，右括号回到初始化器
所在行的缩进。生成器的测试用固定版本的 clang-format 检查输出无需改动。
Arguments of a call share one line when they fit, otherwise every line is filled and
continued aligned after the opening parenthesis; when that leaves no room the call breaks after
the parenthesis and indents by four. A brace initializer that fits stays on one line, otherwise
it has one entry per line, each followed by a comma, and the closing brace returns to the
indent of the line the initializer starts on. The tests of the generator check with a pinned
clang-format that the output needs no changes.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field

from xrobot.source_syntax import close_token, code_tokens, split_arguments

COLUMN_LIMIT = 90
INDENT = 2
CONTINUATION = 4

# 花括号或括号前可以出现的类型写法：限定名，可带模板实参。
# What may precede a brace or a parenthesis: a qualified name with optional template arguments.
_TYPE_PREFIX = re.compile(
    r"[A-Za-z_][A-Za-z_0-9]*(?:\s*::\s*[A-Za-z_][A-Za-z_0-9]*)*(?:\s*<.*>)?", re.S
)
_DESIGNATOR = re.compile(r"\.\s*([A-Za-z_][A-Za-z_0-9]*)\s*=(?!=)")


@dataclass
class Node:
    """初始化器或实参的结构：花括号列表、调用、指定初始化项，或原样保留的文本。
    The structure of an initializer or argument: a brace list, a call, a designated entry, or
    text kept as written.
    """

    kind: str  # 'text' | 'brace' | 'call' | 'designated'
    text: str = ""  # text 的内容；brace 和 call 的前缀；designated 的字段名
    items: list[Node] = field(default_factory=list)


def parse_node(text: str) -> Node:
    """把一段 C++ 文本拆成 Node；只拆顶层的花括号列表、调用和指定初始化项，其余原样保留。
    Split a piece of C++ text into a Node; only top-level brace lists, calls and designated
    entries are split, everything else is kept as written.
    """
    text = text.strip()
    items = code_tokens(text)
    if not items:
        return Node("text", text)
    designator = _DESIGNATOR.match(text)
    if designator and items[0].text == ".":
        return Node("designated", designator.group(1), [parse_node(text[designator.end() :])])
    for opening, kind in (("{", "brace"), ("(", "call")):
        start = next(
            (i for i, t in enumerate(items) if t.text in ("{", "(", "[") and _top_level(items, i)),
            None,
        )
        if start is None or items[start].text != opening:
            continue
        if close_token(items, start) != len(items) - 1:
            continue
        prefix = text[: items[start].start].strip()
        if prefix and not _TYPE_PREFIX.fullmatch(prefix):
            continue
        if kind == "call" and not prefix:
            continue
        body = text[items[start].end : items[-1].start]
        parts = [p for p in split_arguments(body) if p.strip()]
        return Node(kind, prefix, [parse_node(p) for p in parts])
    return Node("text", text)


def _top_level(items: list, index: int) -> bool:
    """items[index] 前面没有未配对的括号（模板尖括号不算）。
    Whether no bracket is open before items[index] (template angle brackets do not count).
    """
    depth = 0
    for token in items[:index]:
        if token.text in ("{", "(", "["):
            depth += 1
        elif token.text in ("}", ")", "]"):
            depth -= 1
    return depth == 0


def flat(node: Node) -> str:
    """node 写在一行的文本。
    The text of node on one line.
    """
    if node.kind == "text":
        return node.text
    if node.kind == "designated":
        return f".{node.text} = {flat(node.items[0])}"
    opening, closing = ("{", "}") if node.kind == "brace" else ("(", ")")
    return node.text + opening + ", ".join(flat(item) for item in node.items) + closing


def _fits(head: str, text: str, trailer: str) -> bool:
    """head、text 和 trailer 接在一起是否不超过列宽，且 text 没有换行。
    Whether head, text and trailer together stay within the column limit and text has no
    line break.
    """
    return "\n" not in text and len(head) + len(text) + len(trailer) <= COLUMN_LIMIT


def layout_value(head: str, text: str, trailer: str, indent: int) -> list[str]:
    """把一个值接在 head 之后排版；返回的第一行以 head 开头，最后一行以 trailer 结尾。
    Lay out a value after head; the first returned line starts with head and the last ends
    with trailer.

    Args:
        head: 值之前已在这一行上的文本（含缩进）。
            The text already on the line before the value, indentation included.
        indent: 这一行的缩进，右花括号回到这里。
            The indentation of the line; the closing brace returns to it.

    全部是指定初始化项的最外层花括号列表每项一行，放得下也一样。
    An outermost brace list of designated entries has one entry per line even when it fits.
    """
    return _layout(parse_node(text), head, trailer, indent, 0, False)


def _layout(node: Node, head: str, trailer: str, indent: int, level: int, force: bool) -> list[str]:
    """layout_value 对解析后的 Node 的递归；level 是花括号的嵌套层数，force 要求展开。
    The recursion of layout_value over a parsed Node; level is the brace nesting depth and
    force requires the node to be expanded.
    """
    text = flat(node)
    designated = (
        node.kind == "brace"
        and bool(node.items)
        and all(item.kind == "designated" for item in node.items)
    )
    if not (force or (level == 0 and designated)) and _fits(head, text, trailer):
        return [head + text + trailer]
    if node.kind == "designated":
        child = node.items[0]
        if child.kind == "brace" and child.items:
            # Allman 花括号：字段名后换行，花括号另起一行并多缩进一级，每项一行。
            # Allman braces: break after the field name, and the brace starts a line one
            # level deeper with one entry per line.
            inner = indent + CONTINUATION
            lines = [head + f".{node.text} ="]
            return lines + _layout(child, " " * inner, trailer, inner, level + 1, True)
        return _layout(child, head + f".{node.text} = ", trailer, indent, level, False)
    if node.kind == "brace" and node.items:
        inner = indent + CONTINUATION
        lines = [head + node.text + "{"]
        rows = _columns([flat(item) for item in node.items], inner, level, node.items)
        if rows is None:
            for item in node.items:
                lines += _layout(item, " " * inner, ",", inner, level + 1, False)
        else:
            lines += rows
        lines.append(" " * indent + "}" + trailer)
        return lines
    if node.kind == "call" and node.items:
        lead = len(head) - len(head.lstrip(" "))
        return layout_call(
            head.lstrip(" ") + node.text + "(", [flat(i) for i in node.items], ")" + trailer, lead
        )
    return [head + text + trailer]


def _columns(items: list[str], indent: int, level: int, nodes: list[Node]) -> list[str] | None:
    """花括号列表的表格排版：clang-format 对有尾随逗号的列表按列对齐，选行数最少的列数。
    The table layout of a brace list: clang-format aligns a list with a trailing comma in
    columns and picks the column count with the fewest lines.

    只有最外层列表有 5 项以上、内层列表有 19 项以上，且每项都能放在一行时才用；返回 None 时
    每项一行。
    It applies only to an outermost list of five items or more, or a nested list of nineteen
    or more, whose items each fit on one line; None means one item per line.
    """
    if len(items) < (5 if level == 0 else 19):
        return None
    if any(
        "\n" in text or node.kind == "designated" for text, node in zip(items, nodes, strict=True)
    ):
        return None
    lengths = [len(text) + 1 for text in items]  # 含逗号 / the comma included
    remaining = COLUMN_LIMIT - indent
    formats = []
    for columns in range(1, COLUMN_LIMIT // 3 + 1):
        sizes = [0] * columns
        smallest = [10**9] * columns
        count = 1
        for i, length in enumerate(lengths):
            column = i % columns
            if i and column == 0:
                count += 1
            sizes[column] = max(sizes[column], length)
            smallest[column] = min(smallest[column], length)
        if columns > len(items):
            break
        width = columns - 1 + sum(sizes)
        if any(sizes[c] - smallest[c] > 10 for c in range(columns - 1)):
            continue
        if width > COLUMN_LIMIT and columns > 1:
            continue
        formats.append((columns, sizes, width, count))
    best = None
    for columns, sizes, width, count in reversed(formats):
        if width <= remaining or columns == 1:
            if best is not None and count > best[3]:
                break
            best = (columns, sizes, width, count)
    if best is None or best[0] == 1:
        return None
    columns, sizes = best[0], best[1]
    rows = []
    for start in range(0, len(items), columns):
        cells = [text + "," for text in items[start : start + columns]]
        padded = [cell.ljust(sizes[c]) for c, cell in enumerate(cells[:-1])] + cells[-1:]
        rows.append(" " * indent + " ".join(padded))
    return rows


def layout_declaration(
    declaration: str, name: str, value: str, trailer: str, indent: int
) -> list[str]:
    """一个带初始值的变量声明：第一行放不下时，在类型和变量名之间换行。
    A variable declaration with an initial value: when its first line is too long, it breaks
    between the type and the name.

    Args:
        declaration: 变量名之前的部分，如 "static const T"。
            The part before the name, such as "static const T".
    """
    prefix = " " * indent
    lines = layout_value(f"{prefix}{declaration} {name} = ", value, trailer, indent)
    if len(lines[0]) <= COLUMN_LIMIT:
        return lines
    inner = indent + CONTINUATION
    first = f"{prefix}{declaration} {name} ="
    if len(first) <= COLUMN_LIMIT:
        # 变量名放得下：值另起一行。
        # The name fits: the value starts a line of its own.
        return [first] + layout_value(" " * inner, value, trailer, inner)
    return [prefix + declaration] + layout_value(" " * inner + f"{name} = ", value, trailer, inner)


def wrap_comment(text: str, indent: int = 0) -> list[str]:
    """一行 // 注释；超过列宽时在空格处折成多行，与 clang-format 的 ReflowComments 一致。
    A // comment line; one longer than the column limit is broken at spaces into several
    lines, as clang-format's ReflowComments does.
    """
    prefix = " " * indent + "// "
    lines, current = [], prefix.rstrip()
    for word in text.split(" "):
        if current != prefix.rstrip() and len(current) + 1 + len(word) > COLUMN_LIMIT:
            lines.append(current)
            current = prefix.rstrip()
        current += " " + word
    lines.append(current)
    return lines


def layout_call(
    head: str,
    arguments: list[str],
    tail: str,
    indent: int = 0,
    extra: int = 0,
    hang_margin: int = 1,
) -> list[str]:
    """一个调用：实参放得下就在一行，否则按 clang-format 填满每一行。
    A call: the arguments share one line when they fit, otherwise every line is filled as
    clang-format does.

    Args:
        head: 到左括号为止的文本，不含缩进；整行的缩进是 indent。
            The text up to and including the opening parenthesis, without indentation.
        arguments: 每个实参的单行文本。
            The one-line text of each argument.
        tail: 最后一个实参之后的文本，如 ")" 或 ");"。
            The text after the last argument, such as ")" or ");".
        extra: 每行末尾之后要占用的宽度（宏的续行符）。
            Width reserved after every line (the line continuation of a macro).
        hang_margin: 左括号后换行比对齐少几行时才换行；函数声明比调用更不愿意换行。
            How many lines breaking after the parenthesis must save before it is chosen; a
            function declaration is more reluctant than a call.
    """
    prefix = " " * indent
    one_line = prefix + head + ", ".join(arguments) + tail
    if len(one_line) <= COLUMN_LIMIT and "\n" not in one_line:
        return [one_line]
    aligned = _pack(prefix + head, arguments, tail, len(prefix + head), extra)
    hanging = [prefix + head] + _pack(
        " " * (indent + CONTINUATION), arguments, tail, indent + CONTINUATION, extra, True
    )
    limit = COLUMN_LIMIT - extra
    if any(len(line) > limit for line in aligned) or len(aligned) - len(hanging) >= hang_margin:
        return hanging
    return aligned


def _pack(
    first: str, arguments: list[str], tail: str, column: int, extra: int, fresh: bool = False
) -> list[str]:
    """把实参填进行：first 是第一行已有的文本，续行缩进 column 格。
    Fill lines with the arguments: first is the text already on the first line, and
    continuation lines are indented by column.

    fresh 为真时 first 只是缩进，第一个实参不加空格。
    With fresh, first is only indentation and the first argument gets no space before it.
    """
    limit = COLUMN_LIMIT - extra
    lines, current = [], first
    start = first
    for i, argument in enumerate(arguments):
        piece = argument + (tail if i + 1 == len(arguments) else ",")
        if current == start:
            current += piece
        elif len(current) + 1 + len(piece) <= limit:
            current += " " + piece
        else:
            lines.append(current)
            start = " " * column
            current = start + piece
    lines.append(current)
    return lines


def layout_macro(name: str, call: str, arguments: list[str]) -> list[str]:
    """#define name() call(arguments...)；放不下时调用另起一行并用续行符对齐。
    #define name() call(arguments...); when it does not fit the call starts on its own line and
    the line continuations are aligned.
    """
    one_line = f"#define {name}() {call}({', '.join(arguments)})"
    if len(one_line) <= COLUMN_LIMIT:
        return [one_line]
    body = layout_call(call + "(", arguments, ")", INDENT, extra=2)
    lines = [f"#define {name}()"] + body
    width = max(len(line) for line in lines[:-1]) + 1
    return [line.ljust(width) + "\\" for line in lines[:-1]] + [lines[-1]]


def sort_includes(names: list[str]) -> list[str]:
    """按 clang-format 的 Regroup 排好的 #include 行：先 <*.h>，再其他 <...>，再 "..."，
    组之间空一行，组内按名字的字节序排序并去重。
    The #include lines grouped as clang-format's Regroup does: <*.h>, other <...>, then "...",
    a blank line between groups, each group sorted bytewise by name without repeats.

    Args:
        names: 每个 include 的写法，如 "libxr.hpp"（带引号）或 <memory>。
            Each include as written, such as "libxr.hpp" with its quotes or <memory>.
    """
    groups: dict[int, list[str]] = {1: [], 2: [], 3: []}
    for name in sorted(set(names), key=lambda n: n.strip('"<>')):
        if re.match(r"<ext/.*\.h>", name):
            groups[2].append(name)
        elif re.match(r"<.*\.h>", name):
            groups[1].append(name)
        elif name.startswith("<"):
            groups[2].append(name)
        else:
            groups[3].append(name)
    lines: list[str] = []
    for group in groups.values():
        if group:
            if lines:
                lines.append("")
            lines += [f"#include {name}" for name in group]
    return lines
