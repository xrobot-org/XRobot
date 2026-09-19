"""xr-syntax-backed C++ interface and XR_REGISTER source queries.

This module is the migration path away from the historical CppSource scanner.
Production callers keep the legacy path until golden parity is complete.
"""

import re
from pathlib import Path
from typing import Dict, List, Optional

_IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z_0-9]*\Z")
_IF_DIRECTIVE = re.compile(r"#\s*(?:if|ifdef|ifndef)\b")
_ENDIF_DIRECTIVE = re.compile(r"#\s*endif\b")


def _global_class(document, name: str):
    """Return the unique explicitly defined global class/struct with this name."""
    candidates = []
    for view in document.class_views(name):
        parent = view.node.parent
        if parent is None:
            continue
        if parent.kind == "translation_unit":
            candidates.append(view)
            continue
        if (
            parent.kind == "template_declaration"
            and parent.parent is not None
            and parent.parent.kind == "translation_unit"
        ):
            candidates.append(view)
    if not candidates:
        raise ValueError(
            "No explicit global class %s; macro-generated interfaces are not supported" % name
        )
    if len(candidates) != 1:
        raise ValueError("Multiple definitions of Module class %s" % name)
    return candidates[0]


def _template_text(view) -> Optional[str]:
    """Return exact source text inside the directly enclosing template<...>."""
    parent = view.node.parent
    if parent is None or parent.kind != "template_declaration":
        return None
    parameters = parent.child_by_field("parameters")
    if parameters is None:
        return None
    text = parameters.text.strip()
    if len(text) >= 2 and text[0] == "<" and text[-1] == ">":
        text = text[1:-1].strip()
    return text or None


def _conditional_depth_before(view, offset: int) -> int:
    """Count unmatched class-body #if directives before a member offset."""
    body = view.body
    if body is None:
        return 0
    depth = 0
    for child in body.syntax_children:
        if child.span.start >= offset:
            break
        text = child.text.lstrip()
        if text.startswith("#") and _IF_DIRECTIVE.match(text):
            depth += 1
        elif text.startswith("#") and _ENDIF_DIRECTIVE.match(text):
            depth = max(0, depth - 1)
    return depth


def extract_interface(source: str, name: str) -> dict:
    """Extract the XRobot source-level Module interface through xr-syntax."""
    from xr_syntax.cpp import CppDocument

    document = CppDocument.parse(source)
    view = _global_class(document, name)
    constructors = []
    encoded = source.encode("utf-8", errors="surrogateescape")
    for function in view.constructors(public_only=True, callable_only=True):
        declarator = function.declarator
        if declarator is None:
            continue
        if _conditional_depth_before(view, function.node.span.start):
            raise ValueError("%s constructor interface varies under #if" % name)
        constructors.append(
            {
                "declaration": declarator.text.strip(),
                "parameters": [parameter.text.strip() for parameter in function.parameters],
                "line": encoded[: declarator.span.start].count(b"\n") + 1,
            }
        )
    if not constructors:
        raise ValueError("No supported explicit public constructor for %s" % name)
    return {
        "name": name,
        "template": _template_text(view),
        "constructors": constructors,
    }


def registrations_from_text(source: str, source_name: Optional[str] = None) -> List[Dict]:
    """Return XR_REGISTER names/types/locations through xr-syntax invocation views."""
    from xr_syntax.cpp import CppDocument

    document = CppDocument.parse(source, source_name=source_name)
    records = []
    names = set()
    for invocation in document.invocation_views("XR_REGISTER", template_angles=True):
        parts = list(invocation.arguments)
        if len(parts) < 2 or not _IDENTIFIER.fullmatch(parts[0]):
            raise ValueError(
                "%s: XR_REGISTER requires an existing name and explicit object types"
                % (source_name or "<source>")
            )
        name = parts[0]
        types = parts[1:]
        if name in names:
            raise ValueError("Duplicate XR_REGISTER name: " + name)
        if name.startswith("xr_"):
            raise ValueError("Registration collides with generated identifier prefix: " + name)
        if len(set(types)) != len(types):
            raise ValueError("Duplicate view in XR_REGISTER: " + name)
        if any(cpp_type.rstrip().endswith("&") for cpp_type in types):
            raise ValueError("Register object types, not reference types: " + name)
        names.add(name)
        records.append(
            {
                "name": name,
                "types": types,
                "source": source_name or "<source>",
                "line": invocation.line,
                "offset": invocation.span.start,
                "char_offset": len(
                    source.encode("utf-8", errors="surrogateescape")[: invocation.span.start]
                    .decode("utf-8", errors="surrogateescape")
                ),
            }
        )
    return records


def read_registrations(paths) -> List[Dict]:
    """Read XR_REGISTER structure; caller-view classification stays in GenerateMain."""
    records = []
    names = set()
    for value in paths:
        path = Path(value)
        source = path.read_text(encoding="utf-8-sig")
        for record in registrations_from_text(source, str(path)):
            if record["name"] in names:
                raise ValueError("Duplicate XR_REGISTER name: " + record["name"])
            names.add(record["name"])
            records.append(record)
    return records


def split_arguments(text: str) -> List[str]:
    """Split a C++ argument/declaration list through xr-syntax."""
    from xr_syntax.cpp import split_source_list

    if not text.strip():
        return []
    try:
        return list(split_source_list(text, template_angles=True))
    except ValueError as error:
        message = str(error)
        if "unbalanced" in message.lower():
            raise ValueError(
                "Unbalanced argument list; parenthesize comparison expressions"
            ) from error
        if "empty argument" in message.lower():
            raise ValueError("Empty argument in C++ declaration") from error
        raise


def bind_identifiers(expression: str, bindings: Dict[str, List[str]]) -> str:
    """Bind unqualified identifiers while preserving every unrelated source byte."""
    from xr_syntax.cpp import identifier_occurrences

    encoded = expression.encode("utf-8", errors="surrogateescape")
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
        edits.append(
            (
                occurrence.span.start,
                occurrence.span.end,
                views[0].encode("utf-8", errors="surrogateescape"),
            )
        )
    for start, end, replacement in reversed(edits):
        encoded = encoded[:start] + replacement + encoded[end:]
    return encoded.decode("utf-8", errors="surrogateescape")
