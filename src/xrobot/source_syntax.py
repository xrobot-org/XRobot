"""XRobot 对 xr-syntax 的薄适配层。

这里仅保留 XRobot 需要的源码级契约：词法 token、分隔符配对、参数列表切分、
预处理条件层数，以及 Module 构造接口提取。C++ 解析全部由 xr-syntax 负责。

Thin XRobot adapter over xr-syntax.  It keeps only XRobot-specific source
contracts while xr-syntax owns all C++ parsing.
"""

from __future__ import annotations

import bisect
import functools
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

Token = CppLexicalToken


@functools.lru_cache(maxsize=65536)
def _cached_tokens(source: str):
    return tuple(_code_tokens(source))


def code_tokens(source: str) -> list[CppLexicalToken]:
    """返回 XRobot 需要的 C++ 代码 token，并保持旧调用方的 list 接口。

    Return public xr-syntax code tokens as a new list (callers may modify it);
    tokenization of a given text is cached because the generator re-reads the
    same type spellings and headers many times.
    """
    return list(_cached_tokens(source))


@functools.lru_cache(maxsize=256)
def parse_document(source: str, source_name: str | None = None) -> CppDocument:
    """Parse C++ source once per (text, name); documents are immutable snapshots."""
    return CppDocument.parse(source, source_name=source_name)


def close_token(items: Sequence[CppLexicalToken], start: int) -> int:
    """返回 opening token 对应的 closing token 索引。

    Return the matching closing-token index using xr-syntax delimiter rules.
    """
    return matching_delimiter(items, start)


def split_arguments(text: str) -> list[str]:
    """按顶层逗号切分 C++ 参数，同时保留模板参数中的逗号。

    Split a C++ argument/declaration list on top-level commas while treating
    template angle brackets as nesting, matching XRobot's historical contract.
    """
    return list(split_source_list(text, template_angles=True))


_DEPTH_CACHE: dict = {}


def _directive_depths(document: CppDocument):
    """(positions, running depth) of the #if/#ifdef/#ifndef/#endif directives."""
    cached = _DEPTH_CACHE.get(id(document))
    if cached is not None and cached[0] is document:
        return cached[1], cached[2]
    deltas = []
    for node in document.root.descendants():  # one walk for all three kinds
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
    if len(_DEPTH_CACHE) > 256:
        _DEPTH_CACHE.clear()
    _DEPTH_CACHE[id(document)] = (document, positions, depths)
    return positions, depths


def conditional_depth(document: CppDocument, start: int, end: int) -> int:
    """返回 [start, end) 内尚未闭合的 #if/#ifdef/#ifndef 层数。

    Count conditional directives opened but not closed between ``start`` and
    ``end`` from xr-syntax preprocessor nodes, so directive spelling stays
    parser-owned. Directive positions are computed once per document.
    """
    positions, depths = _directive_depths(document)
    before = bisect.bisect_left(positions, start)
    last = bisect.bisect_left(positions, end)
    base = depths[before - 1] if before else 0
    return (depths[last - 1] if last else 0) - base if last > before else 0


def extract_interface(source: str, name: str, source_name: str | None = None) -> dict:
    """从全局显式 class/struct 提取 XRobot 所需的构造接口快照。

    Extract the explicit global Module class interface through xr-syntax typed
    views.  This deliberately returns the historical XRobot dictionary shape so
    ConstructorModel can migrate independently of the parser implementation.
    """
    document = parse_document(source, source_name)
    classes = [
        view
        for view in document.class_views(name)
        if _is_global_class(view.node)
    ]
    if not classes:
        raise ValueError(
            "No explicit global class %s; macro-generated interfaces are not supported"
            % name
        )
    if len(classes) != 1:
        raise ValueError("Multiple definitions of Module class %s" % name)

    class_view = classes[0]
    parent = class_view.node.parent
    templated = parent is not None and parent.kind == "template_declaration"
    template_parameters = class_view.template_parameters
    if templated and not template_parameters:
        raise ValueError("Explicit Module template specialization is not supported")

    constructors = class_view.constructors(public_only=True, callable_only=True)
    if not constructors:
        raise ValueError("No supported explicit public constructor for %s" % name)

    source_bytes = document.render_bytes()
    body = class_view.body
    result = []
    for constructor in constructors:
        if body is not None and conditional_depth(
            document, body.span.start, constructor.node.span.start
        ):
            raise ValueError("%s constructor interface varies under #if" % name)

        declarator = constructor.declarator
        result.append(
            {
                "declaration": (
                    constructor.node.text
                    if declarator is None
                    else declarator.text
                ),
                "parameters": [
                    parameter.text.strip() for parameter in constructor.parameters
                ],
                "line": source_bytes[: constructor.node.span.start].count(b"\n") + 1,
            }
        )

    monitor = any(function.name == "OnMonitor" and function.access == "public"
                  for function in class_view.functions())
    return {
        "name": name,
        "monitor": monitor,
        "template": (
            ", ".join(parameter.text for parameter in template_parameters)
            if template_parameters
            else None
        ),
        "constructors": result,
    }


def _is_global_class(node) -> bool:
    """判断 class/struct 是否直接属于 translation unit（允许 template 包装）。

    Return whether a class/struct is a translation-unit declaration, optionally
    wrapped directly by a template declaration.
    """
    parent = node.parent
    if parent is not None and parent.kind == "template_declaration":
        parent = parent.parent
    return parent is not None and parent.kind == "translation_unit"
