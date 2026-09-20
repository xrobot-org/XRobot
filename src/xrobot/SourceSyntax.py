"""XRobot 对 xr-syntax 的薄适配层。

这里仅保留 XRobot 需要的源码级契约：词法 token、分隔符配对、参数列表切分、
注册名绑定，以及 Module 构造接口提取。真正的 C++ 解析由 xr-syntax 负责；
旧 CppSource.py 暂时只用于迁移期 golden parity，不再作为生产读路径。

Thin XRobot adapter over xr-syntax.  It keeps only XRobot-specific source
contracts while xr-syntax owns C++ parsing.  The legacy CppSource.py remains
temporarily for golden-parity checks and is no longer a production read path.
"""

from __future__ import annotations

import re
from typing import Dict, List, Sequence

from xr_syntax.cpp import (
    CppDocument,
    CppLexicalToken,
    code_tokens as _code_tokens,
    identifier_occurrences,
    matching_delimiter,
    split_source_list,
)


Token = CppLexicalToken


def code_tokens(source: str) -> List[CppLexicalToken]:
    """返回 XRobot 需要的 C++ 代码 token，并保持旧调用方的 list 接口。

    Return public xr-syntax code tokens as a list so existing XRobot algorithms
    can migrate without changing their collection semantics.
    """
    return list(_code_tokens(source))


def close_token(items: Sequence[CppLexicalToken], start: int) -> int:
    """返回 opening token 对应的 closing token 索引。

    Return the matching closing-token index using xr-syntax delimiter rules.
    """
    return matching_delimiter(items, start)


def split_arguments(text: str) -> List[str]:
    """按顶层逗号切分 C++ 参数，同时保留模板参数中的逗号。

    Split a C++ argument/declaration list on top-level commas while treating
    template angle brackets as nesting, matching XRobot's historical contract.
    """
    return list(split_source_list(text, template_angles=True))


def bind_identifiers(expression: str, bindings: Dict[str, List[str]]) -> str:
    """替换未限定的注册逻辑名，并保持其余源码字节不变。

    Bind unqualified registered identifiers while leaving member names, scope
    roots, comments, preprocessor lines, literals, and unrelated spelling intact.
    """
    edits = []
    for occurrence in identifier_occurrences(expression):
        if occurrence.text not in bindings:
            continue
        if occurrence.qualified_left or occurrence.scope_root:
            continue
        views = bindings[occurrence.text]
        if len(views) != 1:
            raise ValueError(
                "Ambiguous registered name '%s'; select one of: %s"
                % (occurrence.text, ", ".join(views))
            )
        edits.append((occurrence.span.start, occurrence.span.end, views[0]))

    encoded = expression.encode("utf-8", errors="surrogateescape")
    for start, end, replacement in reversed(edits):
        encoded = (
            encoded[:start]
            + replacement.encode("utf-8", errors="surrogateescape")
            + encoded[end:]
        )
    return encoded.decode("utf-8", errors="surrogateescape")


def extract_interface(source: str, name: str, source_name: str | None = None) -> dict:
    """从全局显式 class/struct 提取 XRobot 所需的构造接口快照。

    Extract the explicit global Module class interface through xr-syntax typed
    views.  This deliberately returns the historical XRobot dictionary shape so
    ConstructorModel can migrate independently of the parser implementation.
    """
    document = CppDocument.parse(source, source_name=source_name)
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
        if body is not None:
            prefix = source_bytes[
                body.span.start : constructor.node.span.start
            ].decode("utf-8", errors="surrogateescape")
            conditional_depth = 0
            for line in prefix.splitlines():
                if re.match(r"^\s*#\s*(if|ifdef|ifndef)\b", line):
                    conditional_depth += 1
                elif re.match(r"^\s*#\s*endif\b", line):
                    conditional_depth -= 1
            if conditional_depth:
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

    return {
        "name": name,
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
