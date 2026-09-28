"""按名称查找模块头文件中的 class/struct，读取字段顺序与构造函数签名。

Read class/struct definitions from loaded Module headers: data-member names in
declaration order, and public constructor signatures. This backs the YAML
completeness rules without evaluating types. A name that may denote something
the index cannot see (a template parameter, a member of a base class that is not
in the loaded headers) is reported as unknown instead of falling back to an
unrelated outer declaration with the same name.

Headers are parsed lazily: only files whose text mentions a requested name are
parsed, and each file is parsed and tokenized once.
"""
from __future__ import annotations

import bisect
import re
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

from xr_syntax.cpp import CppDocument

from xrobot.SourceSyntax import code_tokens, close_token, split_arguments

_SCOPES = ('namespace_definition', 'class_specifier', 'struct_specifier')
_CLASS_KEYS = ('class', 'struct', 'union', 'enum')
_SKIP_LEADING = {'using', 'typedef', 'friend', 'template', 'static_assert', 'operator', '~',
                 'public', 'protected', 'private'}
_SPECIFIERS = {'static', 'inline', 'constexpr', 'consteval', 'constinit', 'mutable',
               'thread_local', 'volatile', 'const', 'explicit', 'virtual'}


def _strip_type(text: str) -> Optional[Tuple[str, ...]]:
    """Return the name path of a class type spelling, or None for pointers/arrays/functions."""
    items = code_tokens(text)
    names, current, i = [], None, 0
    while i < len(items):
        token = items[i]
        if token.text in ('const', 'volatile', 'typename', 'struct', 'class', '&', '&&'):
            i += 1
            continue
        if token.text in ('*', '[', '('):
            return None
        if token.text == '<':
            i = close_token(items, i) + 1
            continue
        if token.text == '::':
            if current is not None:
                names.append(current)
            current = None
            i += 1
            continue
        if token.kind == 'identifier':
            current = token.text
        i += 1
    if current is not None:
        names.append(current)
    return tuple(names) if names else None


def _split_last(spelled: str) -> Tuple[str, Optional[List[str]]]:
    """Split ``A<x>::B<y, z>`` into the outer spelling ``A<x>`` and ``['y', 'z']``."""
    items = code_tokens(spelled)
    cut, i, last_open = None, 0, None
    while i < len(items):
        if items[i].text == '<':
            last_open = i
            i = close_token(items, i) + 1
            continue
        if items[i].text == '::':
            cut = items[i].start
            last_open = None
        i += 1
    outer = spelled[:cut].strip() if cut is not None else ''
    args = None
    if last_open is not None and (cut is None or items[last_open].start > cut):
        close = close_token(items, last_open)
        args = split_arguments(spelled[items[last_open].end:items[close].start])
    return outer, args


def _replace_identifiers(text: str, replacements: Dict[str, str]) -> str:
    items = code_tokens(text)
    edits = []
    for i, token in enumerate(items):
        if token.kind != 'identifier' or token.text not in replacements:
            continue
        if i and items[i - 1].text in ('::', '.', '->'):
            continue
        edits.append((token.start, token.end, replacements[token.text]))
    for start, end, value in reversed(edits):
        text = text[:start] + value + text[end:]
    return text


class _Header:
    """One parsed header: text, syntax document and a single token list.

    Syntax-node spans are byte offsets; lexical tokens use character offsets.
    """

    def __init__(self, path: Path, text: str):
        self.path = path
        self.text = text
        self.document = CppDocument.parse(text, source_name=str(path))
        self.tokens = code_tokens(text)
        byte_of = [0] * (len(text) + 1)
        total = 0
        for i, ch in enumerate(text):
            byte_of[i] = total
            total += len(ch.encode('utf-8', errors='surrogateescape'))
        byte_of[len(text)] = total
        self._byte_of = byte_of
        self._starts = [byte_of[t.start] for t in self.tokens]
        deltas = []
        for kind in ('preproc_if', 'preproc_ifdef', 'preproc_call'):
            for node in self.document.nodes(kind):
                if kind != 'preproc_call':
                    deltas.append((node.span.start, 1))
                    continue
                significant = [c for c in node.syntax_children if not c.is_trivia]
                if len(significant) > 1 and significant[1].text == 'endif':
                    deltas.append((node.span.start, -1))
        deltas.sort()
        self._directive_at = [position for position, _ in deltas]
        depth, running = [], 0
        for _, delta in deltas:
            running += delta
            depth.append(running)
        self._depth = depth

    def tokens_in(self, start: int, end: int):
        lo = bisect.bisect_left(self._starts, start)
        hi = bisect.bisect_left(self._starts, end)
        return self.tokens[lo:hi]

    def index_of(self, token) -> int:
        return bisect.bisect_left(self._starts, self._byte_of[token.start])

    def conditional_depth(self, token) -> int:
        """Open #if/#ifdef/#ifndef levels at a token (whole file)."""
        i = bisect.bisect_right(self._directive_at, self._byte_of[token.start])
        return self._depth[i - 1] if i else 0

    def byte(self, character_offset: int) -> int:
        return self._byte_of[character_offset]


class _Layout:
    """Data members and aggregate-relevant facts of one class body."""

    def __init__(self):
        self.fields: List[Tuple[str, str, str]] = []  # (name, type spelling, access)
        self.field_defaults: Dict[str, str] = {}  # default member initializer text
        self.conditional_fields: List[str] = []
        self.has_union = False
        self.has_virtual = False
        self.member_types: Dict[str, str] = {}
        self.aliases: Dict[str, Tuple[int, int]] = {}
        self.monitor: Optional[str] = None  # 'public' / 'conditional' / None


def _scan_body(header: _Header, items, default_access: str) -> _Layout:
    """Scan one class body's tokens (including its braces)."""
    layout = _Layout()
    access = default_access
    i, end = 1, len(items) - 1
    while i < end:
        start = i
        first = items[i]
        if first.text in ('public', 'protected', 'private') and i + 1 < end and items[i + 1].text == ':':
            access = first.text
            i += 2
            continue
        if first.text == ';':
            i += 1
            continue
        # Collect one member declaration up to its top-level ';' (or a function body).
        j = i
        body_close = None
        while j < end and items[j].text != ';':
            if items[j].text == '{':
                close = close_token(items, j)
                previous = items[j - 1] if j > i else None
                # In a function declaration, a brace right after a member name
                # is a constructor initializer; any other brace opens the body.
                if previous is not None and _is_function(items[i:j]) and (
                        previous.kind != 'identifier'
                        or previous.text in ('const', 'override', 'final', 'noexcept')
                        or any(t.text == '->' for t in items[i:j])) and previous.text != '>':
                    body_close = close  # function body: the member ends here
                    break
                j = close + 1
                continue
            if items[j].text in ('(', '[', '<') and j + 1 < end:
                if items[j].text == '<' and not _template_open(items, i, j):
                    j += 1
                    continue
                j = close_token(items, j) + 1
                continue
            j += 1
        member = items[i:(body_close + 1 if body_close is not None else j)]
        i = (body_close + 1) if body_close is not None else j + 1
        if not member:
            continue
        _classify(header, member, access, layout)
        if start == i:
            i += 1
    return layout


def _template_open(items, start, index) -> bool:
    """Whether ``<`` at ``index`` opens template arguments (vs. a comparison)."""
    previous = items[index - 1] if index > start else None
    return previous is not None and (previous.kind == 'identifier' or previous.text == 'template')


def _classify(header: _Header, member, access: str, layout: _Layout):
    texts = [t.text for t in member]
    first = texts[0]
    if first == 'using':
        if len(texts) >= 4 and texts[2] == '=' and member[1].kind == 'identifier':
            layout.member_types[texts[1]] = 'using'
            layout.aliases[texts[1]] = (member[2].end, member[-1].end)
        elif 'OnMonitor' in texts and access == 'public':
            layout.monitor = 'conditional' if header.conditional_depth(member[0]) else 'public'
        return
    if first == 'typedef':
        # typedef struct {...} Name; is registered as a class by TypeIndex._parse.
        if member[-1].kind == 'identifier':
            layout.member_types[texts[-1]] = 'typedef'
        return
    if first in _SKIP_LEADING:
        return
    if first in _CLASS_KEYS:
        k = 1
        if first == 'enum' and k < len(texts) and texts[k] in ('class', 'struct'):
            k += 1
        name = texts[k] if k < len(texts) and member[k].kind == 'identifier' else None
        opening = next((m for m, t in enumerate(texts) if t == '{'), None)
        if first == 'union':
            layout.has_union = True
        if name is not None and opening is not None:
            layout.member_types[name] = first
        if opening is None:
            return  # forward declaration or elaborated member type handled below
        close = close_token(member, opening)
        rest = member[close + 1:]
        declarators = [t for t in rest if t.text != ';']
        if declarators and first != 'enum':
            # `struct T {...} member;` declares a member of the nested type.
            _record_fields(header, declarators, name or '', access, layout)
        elif declarators and first == 'enum':
            _record_fields(header, declarators, name or 'int', access, layout)
        return
    if 'virtual' in texts:
        layout.has_virtual = True
    # Leading specifiers decide static members; functions have a top-level '('
    # before any initializer.
    leading = set()
    for t in texts:
        if t in _SPECIFIERS:
            leading.add(t)
        elif t in ('[', 'alignas', '__attribute__'):
            continue
        else:
            break
    if 'static' in leading:
        return
    if _is_function(member):
        if 'OnMonitor' in texts and access == 'public':
            layout.monitor = 'conditional' if header.conditional_depth(member[0]) else 'public'
        return
    _record_fields(header, member, None, access, layout)


def _is_function(member) -> bool:
    k = 0
    while k < len(member):
        text = member[k].text
        if text in ('=', '{', ':'):
            return False
        if text == '[' and k + 1 < len(member) and member[k + 1].text == '[':
            k = close_token(member, k) + 1
            continue
        if text in ('alignas', '__attribute__', 'decltype') and k + 1 < len(member) and member[k + 1].text == '(':
            k = close_token(member, k + 1) + 1
            continue
        if text == '<':
            k = close_token(member, k) + 1
            continue
        if text == '(':
            return True
        k += 1
    return False


def _record_fields(header: _Header, member, forced_type, access, layout: _Layout):
    parts, start, k = [], 0, 0
    while k < len(member):
        text = member[k].text
        if text in ('(', '{', '[', '<'):
            if text == '<' and not (k and member[k - 1].kind == 'identifier'):
                k += 1
                continue
            k = close_token(member, k) + 1
            continue
        if text == ',':
            parts.append(member[start:k])
            start = k + 1
        if text == ';':
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
            if m and token.text in ('{', '=', '[', ':'):
                break
            if token.text in ('alignas', '__attribute__') and m + 1 < len(part) and part[m + 1].text == '(':
                m = close_token(part, m + 1) + 1
                continue
            if token.text == '[' and m + 1 < len(part) and part[m + 1].text == '[':
                m = close_token(part, m) + 1
                continue
            if token.text == '<':
                m = close_token(part, m) + 1
                continue
            if token.kind == 'identifier' and token.text not in _SPECIFIERS:
                name_index = m
            m += 1
        if name_index is None:
            continue
        if base_type is None:
            type_tokens = [t for t in part[:name_index]]
            spelled = []
            n = 0
            while n < len(type_tokens):
                token = type_tokens[n]
                if token.text in ('alignas', '__attribute__') and n + 1 < len(type_tokens) and \
                        type_tokens[n + 1].text == '(':
                    n = close_token(type_tokens, n + 1) + 1
                    continue
                if token.text == '[' and n + 1 < len(type_tokens) and type_tokens[n + 1].text == '[':
                    n = close_token(type_tokens, n) + 1
                    continue
                if token.text == 'mutable':
                    n += 1
                    continue
                spelled.append(token)
                n += 1
            base_type = header.text[spelled[0].start:spelled[-1].end].strip() if spelled else ''
        name = part[name_index].text
        layout.fields.append((name, base_type, access))
        rest = part[name_index + 1:]
        if rest and rest[0].text == '=' and len(rest) > 1:
            layout.field_defaults[name] = header.text[rest[1].start:rest[-1].end].strip()
        elif rest and rest[0].text == '{':
            layout.field_defaults[name] = header.text[rest[0].start:rest[-1].end].strip()
        if conditional:
            layout.conditional_fields.append(name)


class ClassEntry:
    """One located class/struct definition."""

    def __init__(self, header: _Header, path: Tuple[str, ...], kind: str, body_range: Tuple[int, int],
                 view=None, head_range: Optional[Tuple[int, int]] = None,
                 template_parameters: Optional[List[str]] = None):
        self.header_info = header
        self.header = header.path
        self.path = path
        self.kind = kind
        self.view = view
        self._body_range = body_range  # token indices of '{' and '}'
        self._head_range = head_range
        self.template_parameters = template_parameters or []
        self._layout = None

    @property
    def qualified(self) -> str:
        return '::'.join(self.path)

    @property
    def default_access(self) -> str:
        return 'public' if self.kind == 'struct' else 'private'

    def body_tokens(self):
        start, end = self._body_range
        return self.header_info.tokens[start:end + 1]

    def _head_tokens(self):
        if self._head_range is None:
            return []
        start, end = self._head_range
        return self.header_info.tokens[start:end]

    def layout(self) -> _Layout:
        if self._layout is None:
            self._layout = _scan_body(self.header_info, self.body_tokens(), self.default_access)
        return self._layout

    def has_base(self) -> bool:
        return any(t.text == ':' for t in self._head_tokens())

    def base_spellings(self) -> List[Tuple[str, str]]:
        """Return (access, type spelling) of each direct base class."""
        head = self._head_tokens()
        colon = next((i for i, t in enumerate(head) if t.text == ':' and
                      not (i + 1 < len(head) and head[i + 1].text == ':') and
                      not (i and head[i - 1].text == ':')), None)
        if colon is None:
            return []
        result, start, i = [], colon + 1, colon + 1
        while i <= len(head):
            if i < len(head) and head[i].text == '<':
                i = close_token(head, i) + 1
                continue
            if i == len(head) or head[i].text == ',':
                words = [t for t in head[start:i] if t.text != 'virtual']
                access = self.default_access
                if words and words[0].text in ('public', 'protected', 'private'):
                    access = words[0].text
                    words = words[1:]
                if words:
                    result.append((access, self.header_info.text[words[0].start:words[-1].end].strip()))
                start = i + 1
            i += 1
        return result

    def constructors(self) -> List[List[dict]]:
        if self.view is None:
            return []
        from xrobot.ConstructorModel import parameter
        return [[parameter(p.text) for p in ctor.parameters if p.text.strip() not in ('', 'void')]
                for ctor in self.view.constructors(public_only=True, callable_only=True)]

    def declares_constructor(self) -> bool:
        return self.view is not None and bool(self.view.constructors())

    def fields(self) -> List[Tuple[str, str, str]]:
        """Return (name, type spelling, access) of non-static data members in order."""
        return list(self.layout().fields)

    def member_types(self) -> Dict[str, str]:
        return dict(self.layout().member_types)

    def is_aggregate(self) -> bool:
        layout = self.layout()
        if self.declares_constructor() or self.has_base() or layout.has_virtual:
            return False
        return all(access == 'public' for _, _, access in layout.fields)

    def mapping_problem(self) -> Optional[str]:
        """Why a YAML mapping cannot be checked against this type, or None."""
        layout = self.layout()
        if layout.has_union:
            return '%s contains a union' % self.qualified
        if layout.conditional_fields:
            return '%s declares fields under #if (%s)' % (self.qualified, ', '.join(layout.conditional_fields))
        if not self.declares_constructor() and (self.has_base() or layout.has_virtual):
            return '%s has base classes or virtual functions and no constructor' % self.qualified
        return None

    def monitor(self) -> Optional[str]:
        return self.layout().monitor


class TypeIndex:
    """Lazy index of class/struct definitions in loaded Module headers."""

    def __init__(self, headers: Iterable[Path]):
        self.headers = sorted({Path(h).resolve() for h in headers})
        self._texts: Dict[Path, str] = {}
        self._parsed: Dict[Path, _Header] = {}
        self._entries: Dict[Tuple[str, ...], List[ClassEntry]] = {}
        self._aliases: Dict[Tuple[str, ...], Tuple[_Header, int, int]] = {}
        self._namespace_types: Dict[Tuple[str, ...], set] = {}
        self._ensured: set = set()

    @classmethod
    def for_modules(cls, modules: dict) -> 'TypeIndex':
        return cls(module_headers(modules))

    def _text(self, path: Path) -> str:
        if path not in self._texts:
            self._texts[path] = path.read_text(encoding='utf-8', errors='surrogateescape')
        return self._texts[path]

    def global_class_names(self) -> set:
        """Top-level class/struct names in the loaded headers (textual scan)."""
        pattern = re.compile(r'^(?:template\s*<[^;{]*>\s*)?(?:class|struct)\s+'
                             r'(?:\[\[[^\]]*\]\]\s*|alignas\s*\([^)]*\)\s*)*([A-Za-z_]\w*)', re.M)
        names = set()
        for path in self.headers:
            names.update(pattern.findall(self._text(path)))
        return names

    def _ensure(self, name: str):
        if name in self._ensured:
            return
        self._ensured.add(name)
        pattern = re.compile(r'\b%s\b' % re.escape(name))
        for path in self.headers:
            if path not in self._parsed and pattern.search(self._text(path)):
                self._parse(path)

    def _scope_of(self, header: _Header, byte_position: int) -> Tuple[str, ...]:
        """Enclosing namespace/class names of a byte position (outermost first)."""
        if not hasattr(header, 'scopes'):
            header.scopes = []
            for kind in _SCOPES:
                for node in header.document.nodes(kind):
                    body = node.child_by_field('body')
                    if body is None:
                        continue
                    name = node.child_by_field('name')
                    header.scopes.append((body.span.start, body.span.end,
                                          name.text.strip() if name is not None else ''))
            header.scopes.sort()
        return tuple(name for start, end, name in header.scopes if start < byte_position < end)

    def _add(self, entry: ClassEntry):
        self._entries.setdefault(entry.path, []).append(entry)
        for alias, (start, end) in entry.layout().aliases.items():
            self._aliases.setdefault(entry.path + (alias,), (entry.header_info, start, end))

    def _parse(self, path: Path):
        header = _Header(path, self._text(path))
        self._parsed[path] = header
        for view in header.document.class_views():
            if view.body is None or not view.name:
                continue
            node = view.node
            scope = self._scope_of(header, node.span.start)
            body_tokens = header.tokens_in(view.body.span.start, view.body.span.end)
            if not body_tokens:
                continue
            open_index = header.index_of(body_tokens[0])
            close_index = header.index_of(body_tokens[-1])
            head_tokens = header.tokens_in(node.span.start, view.body.span.start)
            head_range = (header.index_of(head_tokens[0]), open_index) if head_tokens else None
            parameters = [p.name for p in view.template_parameters if getattr(p, 'name', None)]
            kind = 'struct' if node.kind == 'struct_specifier' else 'class'
            self._add(ClassEntry(header, scope + (view.name,), kind, (open_index, close_index),
                                 view=view, head_range=head_range, template_parameters=parameters))
        # typedef struct {...} Name; at namespace scope (xr-syntax does not model it).
        items = header.tokens
        for k, token in enumerate(items[:-2]):
            if token.text != 'typedef' or items[k + 1].text not in ('struct', 'class'):
                continue
            opening = next((m for m in range(k + 2, min(k + 6, len(items))) if items[m].text == '{'), None)
            if opening is None:
                continue
            closing = close_token(items, opening)
            names = [t.text for t in items[closing + 1:closing + 4] if t.kind == 'identifier']
            if not names:
                continue
            scope = self._scope_of(header, header.byte(token.start))
            if scope + (names[0],) not in self._entries:
                self._add(ClassEntry(header, scope + (names[0],), 'struct', (opening, closing)))
        for namespace in header.document.nodes('namespace_definition'):
            name = namespace.child_by_field('name')
            body = namespace.child_by_field('body')
            if name is None or body is None:
                continue
            scope = self._scope_of(header, namespace.span.start) + (name.text.strip(),)
            layout = _scan_namespace(header, header.tokens_in(body.span.start, body.span.end))
            self._namespace_types.setdefault(scope, set()).update(layout.member_types)
            for alias, (start, end) in layout.aliases.items():
                self._aliases.setdefault(scope + (alias,), (header, start, end))

    # -- lookup -------------------------------------------------------------

    def _class_at(self, path: Tuple[str, ...]) -> Optional[ClassEntry]:
        entries = self._entries.get(path)
        if not entries:
            return None
        headers = sorted({str(e.header) for e in entries})
        if len(headers) > 1:
            raise ValueError('Type %s is defined in several Module headers: %s'
                             % ('::'.join(path), ', '.join(headers)))
        return entries[0]

    def resolve(self, spelling: str, scope: Tuple[str, ...] = (), _depth: int = 0) -> Optional[ClassEntry]:
        """Find the class a type spelling names, searching outward from ``scope``.

        Returns None when the name is not found or when it may denote something
        outside the index (a template parameter of an enclosing class, or a
        member of a base class that is not in the loaded headers).
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

    def _lookup_in(self, path, names, owner, depth):
        """Look up ``names`` directly inside ``path`` and, for a class, its bases."""
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
        for access, base in owner.base_spellings():
            parent = self.resolve(base, path[:-1], depth + 1)
            if parent is None:
                return None, False
            found, certain = self._lookup_in(parent.path, names, parent, depth + 1)
            if found is not None or not certain:
                return found, certain
        return None, True

    def qualify_in(self, spelling: str, entry: ClassEntry, entry_spelled: str) -> str:
        """Qualify names in ``spelling`` declared in ``entry``'s class/namespace chain.

        ``entry_spelled`` is how generated code names ``entry`` (it may carry
        template arguments, e.g. ``Outer<T>::Param``). Template parameters of the
        enclosing class templates are replaced by those arguments.
        """
        scopes = []  # (declared names, spelled prefix), innermost first
        replacements = {}
        spelled, path = entry_spelled, entry.path
        while path:
            outer, args = _split_last(spelled) if spelled else ('', None)
            owner = self._class_at(path)
            if owner is not None:
                scopes.append((set(owner.member_types()), spelled))
                if owner.template_parameters and args is not None:
                    for name, value in zip(owner.template_parameters, args):
                        replacements.setdefault(name, value.strip())
            else:
                scopes.append((self._namespace_types.get(path, set()), '::'.join(path)))
            path = path[:-1]
            spelled = outer or '::'.join(path)
        spelling = _replace_identifiers(spelling, replacements)
        items = code_tokens(spelling)
        edits = []
        for i, token in enumerate(items):
            if token.kind != 'identifier':
                continue
            if i and items[i - 1].text in ('::', '.', '->'):
                continue
            for names, prefix in scopes:
                if token.text in names and prefix:
                    edits.append((token.start, token.end, prefix + '::' + token.text))
                    break
        for start, end, text in reversed(edits):
            spelling = spelling[:start] + text + spelling[end:]
        return spelling

    def provides_monitor(self, entry: ClassEntry, _depth: int = 0) -> Optional[bool]:
        """Whether ``entry`` or a public base declares a public OnMonitor.

        Raises when OnMonitor is declared under #if; returns None when a public
        base cannot be located (LibXR bases never provide OnMonitor).
        """
        state = entry.monitor()
        if state == 'conditional':
            raise ValueError('%s declares OnMonitor under #if; the generator cannot evaluate '
                             'build options' % entry.qualified)
        if state == 'public':
            return True
        if _depth > 8:
            return None
        unknown = False
        for access, base in entry.base_spellings():
            if access != 'public':
                continue
            if base.lstrip(':').startswith('LibXR::'):
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


def _scan_namespace(header: _Header, items) -> _Layout:
    """Collect type names and aliases declared directly in a namespace body."""
    layout = _Layout()
    i, end = 1, len(items) - 1
    while i < end:
        t = items[i]
        if t.text in ('{', '(', '[', '<'):
            i = close_token(items, i) + 1
            continue
        if t.text in _CLASS_KEYS:
            j = i + 1
            if t.text == 'enum' and j < end and items[j].text in ('class', 'struct'):
                j += 1
            if j < end and items[j].kind == 'identifier':
                layout.member_types[items[j].text] = t.text
        elif t.text == 'using' and i + 2 < end and items[i + 1].kind == 'identifier' and items[i + 2].text == '=':
            k = i + 3
            while k < end and items[k].text != ';':
                if items[k].text in ('{', '(', '[', '<'):
                    k = close_token(items, k) + 1
                    continue
                k += 1
            layout.member_types[items[i + 1].text] = 'using'
            layout.aliases[items[i + 1].text] = (items[i + 2].end, items[k].start)
        elif t.text == 'typedef':
            k = i + 1
            while k < end and items[k].text != ';':
                if items[k].text in ('{', '(', '[', '<'):
                    k = close_token(items, k) + 1
                    continue
                k += 1
            if items[k - 1].kind == 'identifier':
                layout.member_types[items[k - 1].text] = 'typedef'
        i += 1
    return layout


def module_headers(modules: dict) -> List[Path]:
    """Every header the generator may read for a set of modules."""
    headers = []
    for module in modules.values():
        headers.extend(sorted(Path(module['path']).glob('*.hpp')))
    return headers
