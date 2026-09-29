"""Source-declared constructor contracts and explicit initializer syntax.

Only named, explicit declarations are supported. This is not a C++ type system:
configuration trees come from initializer expressions, and a YAML mapping is
checked against the class definition read from a loaded Module header
(xrobot.type_index) or, for a type the index cannot locate, against the
parameter's designated default initializer. The generator rejects a value only
when the problem is certain; everything else is left to the C++ compiler.
"""

import re

from xrobot.config import ConfigError, value_text
from xrobot.source_syntax import close_token, code_tokens, split_arguments

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


def parameter(declaration):
    items = code_tokens(declaration)
    split = next((t for t in items if t.text == "="), None)
    head = declaration[: split.start].strip() if split else declaration.strip()
    default = declaration[split.end :].strip() if split else None
    parts = code_tokens(head)
    if not parts or parts[-1].kind != "identifier" or len(parts) < 2:
        raise ValueError("Constructor parameters must have explicit names: " + declaration)
    name = parts[-1].text
    cpp_type = head[: parts[-1].start].strip()
    # Type template parameters use the same named declaration representation.
    if name in ("const", "volatile") and cpp_type not in ("class", "typename"):
        raise ValueError("Unsupported parameter declaration: " + declaration)
    if not cpp_type or "(" in cpp_type or "[" in cpp_type or "..." in head:
        raise ValueError("Use an explicit named type alias for this declaration: " + declaration)
    return {"name": name, "type": cpp_type, "default": default, "declaration": declaration}


def class_symbols(source, name):
    """Read visible names, not aggregate members or arbitrary template semantics."""
    ts = code_tokens(source)
    begin = next(
        i for i, t in enumerate(ts[:-1]) if t.text in ("class", "struct") and ts[i + 1].text == name
    )
    opening = next(i for i in range(begin + 2, len(ts)) if ts[i].text == "{")
    ending = close_token(ts, opening)
    access = "public" if ts[begin].text == "struct" else "private"
    symbols, aliases = {}, {}
    i = opening + 1
    while i < ending:
        t = ts[i]
        if t.text in ("public", "protected", "private") and ts[i + 1].text == ":":
            access = t.text
            i += 2
            continue
        if t.text == "using" and ts[i + 1].kind == "identifier" and ts[i + 2].text == "=":
            end = next(j for j in range(i + 3, ending) if ts[j].text == ";")
            symbols[ts[i + 1].text] = access
            aliases[ts[i + 1].text] = source[ts[i + 2].end : ts[end].start].strip()
            i = end + 1
            continue
        if t.text in ("struct", "class", "enum"):
            j = i + 1
            if t.text == "enum" and ts[j].text in ("class", "struct"):
                j += 1
            if ts[j].kind == "identifier":
                symbols[ts[j].text] = access
            while j < ending and ts[j].text not in ("{", ";"):
                j += 1
            if j < ending and ts[j].text == "{":
                end = close_token(ts, j)
                if i and ts[i - 1].text == "typedef" and ts[end + 1].kind == "identifier":
                    symbols[ts[end + 1].text] = access
                # Names in unscoped enums are also visible in the class scope.
                if t.text == "enum" and ts[i + 1].text not in ("class", "struct"):
                    body = source[ts[j].end : ts[end].start]
                    for item in split_arguments(body.rstrip().rstrip(",")):
                        ident = code_tokens(item)[0]
                        if ident.kind == "identifier":
                            symbols[ident.text] = access
                i = end + 1
                continue
        if t.text == "static":
            j = i + 1
            while j < ending and ts[j].text not in (";", "{", "="):
                if ts[j].text == "(":
                    if ts[j - 1].kind == "identifier":
                        symbols[ts[j - 1].text] = access
                    break
                j += 1
            if j < ending and ts[j].text in ("=", "{") and ts[j - 1].kind == "identifier":
                symbols[ts[j - 1].text] = access
        if t.text in ("{", "("):
            i = close_token(ts, i) + 1
            continue
        i += 1
    return symbols, aliases


def enrich_interface(source, interface):
    symbols, aliases = class_symbols(source, interface["name"])
    interface["symbols"] = symbols
    interface["aliases"] = aliases
    for ctor in interface["constructors"]:
        declarations = ctor["parameters"]
        if declarations == ["void"]:
            declarations = []
        ctor["arguments"] = [parameter(p) for p in declarations]
    interface["template_parameters"] = (
        [parameter(p) for p in split_arguments(interface["template"])]
        if interface["template"]
        else []
    )
    return interface


def replace_names(text, replacements):
    # Scope roots such as Mode::VALUE and T::value_type must be qualified here.
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


def template_bindings(interface, supplied):
    parameters = interface["template_parameters"]
    if len(supplied) > len(parameters):
        raise ValueError("Too many template arguments for " + interface["name"])
    replacements = {}
    for i, p in enumerate(parameters):
        value = supplied[i] if i < len(supplied) else p["default"]
        if value is None:
            raise ValueError(
                "Template argument {}.{} must be specified".format(interface["name"], p["name"])
            )
        replacements[p["name"]] = replace_names(str(value), replacements)
    return replacements


def qualify(text, interface, cpp_class, templates=None, expand_aliases=False):
    if text is None:
        return None
    substitutions = dict(templates or {})
    for symbol, access in interface["symbols"].items():
        if access == "public":
            substitutions[symbol] = cpp_class + "::" + symbol
    # Explicit type aliases are followed only when their text is available.
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
            raise ValueError(
                "Expression uses non-public member {}::{}".format(interface["name"], token.text)
            )
    return replace_names(text, substitutions)


def initializer_tree(expression, expected_type=None):
    """Expand a brace initializer's explicit entries; never inspect a type's fields.

    Returns a dict for designated initializers, a list for positional ones, and
    the original text for anything else (named constants, factory calls, casts).
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


def compliant_constructors(interface, cpp_class=None, templates=None):
    """Accept the agreed constructor shape: dependencies first, then defaults."""
    accepted, rejected = [], []
    for ctor in interface["constructors"]:
        config_started = False
        problems = []
        for p in ctor["arguments"]:
            if p["default"] is not None:
                config_started = True
            elif config_started:
                problems.append(
                    "{}: dependency without a default appears after value configuration".format(
                        p["name"]
                    )
                )
        if problems:
            rejected.append("line {}: {}".format(ctor.get("line", "?"), "; ".join(problems)))
        else:
            accepted.append(ctor)
    if not accepted:
        raise ValueError(
            "{}: no compliant constructor; {}".format(interface["name"], " | ".join(rejected))
        )
    return accepted


def type_shape(cpp_type):
    """Split only outer cv/pointer/ref declarators, never template arguments."""
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


def is_arithmetic(cpp_type):
    """Builtin arithmetic types and their <cstdint>/<cstddef> spellings, by value."""
    base, _, pointers, reference = type_shape(cpp_type)
    if pointers or reference:
        return False
    words = [t.text for t in code_tokens(cpp_type) if t.text not in ("const", "volatile", "::")]
    if words and words[0] == "std":
        words = words[1:]
    return bool(words) and all(w in ARITHMETIC for w in words)


def is_dependency(p):
    """A dependency parameter: reference or pointer type without a default (§2.2)."""
    _, _, pointers, reference = type_shape(p["type"])
    return p["default"] is None and bool(pointers or reference)


def explicit_expression_type(value):
    """Recognize explicit casts/initializers and a small portable literal subset."""
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


def constructor_for(interface, named_values, known, cpp_class, templates):
    """Select the constructor whose parameter names the configuration lists.

    ``known`` maps registration names and earlier instance ids to their types.
    Several constructors with the same names are told apart by explicit types
    only (a known name's type, a cast, a typed initializer or a literal).
    """
    names = [next(iter(v)) for v in named_values]
    candidates = []
    supported = compliant_constructors(interface, cpp_class, templates)
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
        raise ValueError(
            "named arguments ({}) do not match any constructor of {}; expected one of: {}".format(
                ", ".join(names),
                interface["name"],
                " | ".join(
                    "(" + ", ".join(p["name"] for p in c["arguments"]) + ")" for c in supported
                ),
            )
        )
    raise ValueError(
        "{}: constructor is ambiguous for the supplied names and explicit types".format(
            interface["name"]
        )
    )


def _base_spelling(cpp_type):
    """Drop outer cv/reference from a type spelling, keeping template arguments."""
    items = code_tokens(cpp_type)
    keep = [t for t in items if t.text not in ("const", "volatile", "&", "&&")]
    if not keep:
        return cpp_type.strip()
    return cpp_type[keep[0].start : keep[-1].end].strip()


def _is_positional_brace(text):
    """``{a, b}`` or ``T{a, b}`` with elements (not designated, not empty)."""
    ts = code_tokens(text)
    opening = next((i for i, t in enumerate(ts) if t.text == "{"), None)
    if opening is None or close_token(ts, opening) != len(ts) - 1:
        return False
    prefix = text[: ts[opening].start].strip()
    if prefix and not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9:]*(?:\s*<.*>)?", prefix, re.S):
        return False
    inner = ts[opening + 1 : -1]
    return bool(inner) and not (len(inner) >= 2 and inner[0].text == ".")


def _is_designated_brace(text):
    """``{.a = 1}`` or ``T{.a = 1}``."""
    ts = code_tokens(text)
    opening = next((i for i, t in enumerate(ts) if t.text == "{"), None)
    return (
        opening is not None
        and opening + 1 < len(ts)
        and ts[opening + 1].text == "."
        and close_token(ts, opening) == len(ts) - 1
    )


class ValueChecker:
    """Render YAML values as C++ while enforcing the mapping rules.

    For a type located in the loaded Module headers a mapping must name exactly
    its data members in order (aggregates) or one public constructor's
    parameters (classes); positional lists and positional brace text are
    rejected. For a type the index cannot locate, a mapping must match the
    parameter's designated default initializer, otherwise it is rejected and
    the value must be written as a C++ expression.
    """

    def __init__(self, index):
        self.index = index
        self.checks = []  # static_assert declarations for the value being rendered

    def _locate(self, cpp_type, scope):
        if not (self.index and cpp_type):
            return None
        return self.index.resolve(cpp_type, scope)

    def render(self, value, field, cpp_type=None, scope=(), default=None):
        """Return (C++ expression, whether it is already a typed expression)."""
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
    def _require(field, keys, expected, what):
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
                "{}: {} ({}); expected: {}".format(
                    field, "; ".join(detail), what, ", ".join(expected)
                )
            )
        raise ConfigError(
            "{}: fields out of declaration order ({}); expected: {}".format(
                field, what, ", ".join(expected)
            )
        )

    def _mapping(self, value, field, cpp_type, scope, default):
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
                "{}: cannot verify the fields of {} in the loaded Module headers; "
                "write this value as a complete C++ expression".format(
                    field, cpp_type or "this value"
                )
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
        return "{}(\n{}\n)".format(spelled, "\n, ".join(args)), True

    def _designated(self, value, field, types, entry, default):
        parts = []
        for key, child in value.items():
            child_default = default.get(key) if isinstance(default, dict) else None
            child_type = types.get(key)
            scope = entry.path if entry is not None else ()
            expr, _ = self.render(child, field + "." + key, child_type, scope, child_default)
            parts.append(f".{key} = {expr}")
        return "{\n" + "\n, ".join(parts) + "\n}" if parts else "{}"


def convert(expr, target, exact, checks, message="value"):
    """Apply the generator's conversion rule for one value.

    Returns (expression, checks). Generator-built typed expressions and braced
    initializers are exact and returned unchanged (a braced list is emitted as
    ``T{...}``). Arithmetic targets go through ``Implicit<P>`` so constant
    conversions keep the compiler's warnings; every other target gets an
    implicit-convertibility check plus a ``static_cast`` that keeps prvalue
    elision and rejects downcasts and explicit-only conversions.
    """
    value_type = _base_spelling(target)
    stripped = expr.lstrip()
    if exact:
        if stripped.startswith("{"):
            return f"std::remove_cv_t<std::remove_reference_t<{target}>>{expr}", checks
        return expr, checks
    if stripped.startswith("{"):
        return f"std::remove_cv_t<std::remove_reference_t<{target}>>{expr}", checks
    if is_arithmetic(target):
        return f"xrobot_generated::Implicit<{value_type}>({expr})", checks
    # The same type (e.g. an immovable factory prvalue) needs no conversion.
    checks.append(
        "static_assert(std::is_same_v<std::remove_cvref_t<decltype(({}))>, "
        "std::remove_cvref_t<{}>> ||\n"
        "              std::is_convertible_v<decltype(({})), {}>,\n"
        '              "{} requires an implicit conversion to {}");'.format(
            expr, target, expr, target, message, target.replace('"', "'")
        )
    )
    return f"static_cast<{target}>({expr})", checks
