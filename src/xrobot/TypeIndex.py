"""按名称查找模块头文件中的 class/struct，读取字段顺序与构造函数签名。

Read class/struct definitions from loaded Module headers: data-member names in
declaration order, and public constructor signatures. This backs the YAML
completeness rules (every mapping key set must be verifiable) without
evaluating types: names are matched by scope path, template arguments are
ignored, and anything that cannot be located is reported as unknown.

Headers are parsed lazily: only files whose text mentions a requested name are
parsed, and each file is parsed and tokenized once.
"""
from __future__ import annotations

import bisect
import re
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple

from xr_syntax.cpp import CppDocument

from xrobot.SourceSyntax import code_tokens, close_token

_SCOPES = ('namespace_definition', 'class_specifier', 'struct_specifier')
_SKIP_MEMBER = {'static', 'using', 'typedef', 'friend', 'template', 'static_assert',
                'enum', 'struct', 'class', 'union', 'constexpr', 'virtual', 'explicit',
                'operator', '~'}


def _cut(text: str, start: int, end: int) -> str:
    """Slice by lexical token offsets (character offsets into ``text``)."""
    return text[start:end]


def _scope_path(node) -> Tuple[str, ...]:
    names = []
    parent = node.parent
    while parent is not None:
        if parent.kind in _SCOPES:
            name = parent.child_by_field('name')
            names.append(name.text.strip() if name is not None else '')
        parent = parent.parent
    return tuple(reversed(names))


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


def _outer_spelling(spelled: str) -> str:
    """Drop the last top-level ``::name`` component of a type spelling."""
    items = code_tokens(spelled)
    cut, i = None, 0
    while i < len(items):
        if items[i].text == '<':
            i = close_token(items, i) + 1
            continue
        if items[i].text == '::':
            cut = items[i].start
        i += 1
    return _cut(spelled, 0, cut).strip() if cut is not None else ''


def _declarator_names(text: str) -> List[Tuple[str, str]]:
    """Split one data-member declaration into (name, type spelling) pairs."""
    items = code_tokens(text.rstrip().rstrip(';'))
    if not items or items[0].text in _SKIP_MEMBER:
        return []
    parts, start, i = [], 0, 0
    while i < len(items):
        if items[i].text in ('(', '{', '[', '<'):
            if items[i].text == '(':
                return []  # function declaration
            i = close_token(items, i) + 1
            continue
        if items[i].text == ',':
            parts.append(items[start:i])
            start = i + 1
        i += 1
    parts.append(items[start:])
    result, base_type = [], None
    for part in parts:
        name_index = None
        for j, token in enumerate(part):
            if j and token.text in ('{', '=', '[', ':'):
                break
            if token.kind == 'identifier':
                name_index = j
        if name_index is None:
            return []
        if base_type is None:
            base_type = _cut(text, part[0].start, part[name_index].start).strip() if name_index else ''
        result.append((part[name_index].text, base_type))
    return result


class _Header:
    """One parsed header: text, syntax document and a single token list.

    Syntax-node spans are byte offsets; lexical tokens use character offsets.
    ``tokens_in`` takes a byte span and ``text_of`` takes token offsets.
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
        self._starts = [byte_of[t.start] for t in self.tokens]

    def text_of(self, start: int, end: int) -> str:
        return self.text[start:end]

    def tokens_in(self, start: int, end: int):
        lo = bisect.bisect_left(self._starts, start)
        hi = bisect.bisect_left(self._starts, end)
        return self.tokens[lo:hi]


def _direct_declarations(items) -> Tuple[Dict[str, str], Dict[str, Tuple[int, int]]]:
    """Scan one scope body's tokens (including its braces) for directly declared names.

    Return the declared type/alias names and, for ``using X = ...;``, the character
    range of the alias target.
    """
    names, aliases = {}, {}
    i = 1
    while i < len(items) - 1:
        t = items[i]
        if t.text in ('{', '(', '[', '<'):
            i = close_token(items, i) + 1
            continue
        if t.text in ('struct', 'class', 'enum', 'union'):
            j = i + 1
            if t.text == 'enum' and items[j].text in ('class', 'struct'):
                j += 1
            if items[j].kind == 'identifier':
                names[items[j].text] = t.text
        elif t.text == 'using' and items[i + 1].kind == 'identifier' and items[i + 2].text == '=':
            end = i + 3
            while end < len(items) and items[end].text != ';':
                if items[end].text in ('{', '(', '[', '<'):
                    end = close_token(items, end) + 1
                    continue
                end += 1
            names[items[i + 1].text] = 'using'
            aliases[items[i + 1].text] = (items[i + 2].end, items[end].start)
        elif t.text == 'typedef':
            k = i + 1
            while k < len(items) and items[k].text != ';':
                if items[k].text in ('{', '(', '[', '<'):
                    k = close_token(items, k) + 1
                    continue
                k += 1
            if items[k - 1].kind == 'identifier':
                names[items[k - 1].text] = 'typedef'
        i += 1
    return names, aliases


class ClassEntry:
    """One located class/struct definition."""

    def __init__(self, view, header: _Header, path: Tuple[str, ...]):
        self.view = view
        self.header_info = header
        self.header = header.path
        self.path = path

    @property
    def qualified(self) -> str:
        return '::'.join(self.path)

    def slice(self, start: int, end: int) -> str:
        """Text between two lexical token offsets of this header."""
        return self.header_info.text_of(start, end)

    def _body_tokens(self):
        body = self.view.body
        return self.header_info.tokens_in(body.span.start, body.span.end) if body is not None else []

    def _head_tokens(self):
        body = self.view.body
        if body is None:
            return []
        return self.header_info.tokens_in(self.view.node.span.start, body.span.start)

    def has_base(self) -> bool:
        return any(t.text == ':' for t in self._head_tokens())

    def base_spellings(self) -> List[Tuple[str, str]]:
        """Return (access, type spelling) of each direct base class."""
        head = self._head_tokens()
        colon = next((i for i, t in enumerate(head) if t.text == ':'), None)
        if colon is None:
            return []
        result, start, i = [], colon + 1, colon + 1
        while i <= len(head):
            if i < len(head) and head[i].text == '<':
                i = close_token(head, i) + 1
                continue
            if i == len(head) or head[i].text == ',':
                words = [t for t in head[start:i] if t.text != 'virtual']
                access = self.view.default_access
                if words and words[0].text in ('public', 'protected', 'private'):
                    access = words[0].text
                    words = words[1:]
                if words:
                    result.append((access, self.slice(words[0].start, words[-1].end).strip()))
                start = i + 1
            i += 1
        return result

    def constructors(self) -> List[List[dict]]:
        from xrobot.ConstructorModel import parameter
        return [[parameter(p.text) for p in ctor.parameters if p.text.strip() not in ('', 'void')]
                for ctor in self.view.constructors(public_only=True, callable_only=True)]

    def declares_constructor(self) -> bool:
        return bool(self.view.constructors())

    def public_member_function(self, name: str) -> bool:
        return any(f.name == name and f.access == 'public' for f in self.view.functions())

    def fields(self) -> List[Tuple[str, str, str]]:
        """Return (name, type spelling, access) of non-static data members in order."""
        result = []
        body = self.view.body
        if body is None:
            return result
        access = self.view.default_access
        previous_type = None  # `struct T { ... } member;` is split into T and `member;`
        for child in body.named_children:
            if child.kind == 'access_specifier':
                access = child.text.strip().rstrip(':').strip()
                previous_type = None
                continue
            if child.kind == 'comment':
                continue
            if child.kind in _SCOPES:
                name = child.child_by_field('name')
                previous_type = name.text.strip() if name is not None else None
                continue
            text = child.text.strip()
            if text.endswith(';'):
                for name, spelling in _declarator_names(text):
                    result.append((name, spelling or previous_type or '', access))
            previous_type = None
        return result

    def is_aggregate(self) -> bool:
        if self.declares_constructor() or self.has_base():
            return False
        return all(access == 'public' for _, _, access in self.fields())

    def member_types(self) -> Dict[str, str]:
        """Names of nested types, enums and aliases declared directly in this class."""
        return _direct_declarations(self._body_tokens())[0]


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
        headers = []
        for module in modules.values():
            headers.extend(Path(module['path']).glob('*.hpp'))
        return cls(headers)

    def _text(self, path: Path) -> str:
        if path not in self._texts:
            self._texts[path] = path.read_text(encoding='utf-8', errors='surrogateescape')
        return self._texts[path]

    def _ensure(self, name: str):
        """Parse every not-yet-parsed header whose text mentions ``name``."""
        if name in self._ensured:
            return
        self._ensured.add(name)
        pattern = re.compile(r'\b%s\b' % re.escape(name))
        for path in self.headers:
            if path not in self._parsed and pattern.search(self._text(path)):
                self._parse(path)

    def _parse(self, path: Path):
        header = _Header(path, self._text(path))
        self._parsed[path] = header
        for view in header.document.class_views():
            if view.body is None or not view.name:
                continue
            entry = ClassEntry(view, header, _scope_path(view.node) + (view.name,))
            self._entries.setdefault(entry.path, []).append(entry)
            for alias, (start, end) in _direct_declarations(entry._body_tokens())[1].items():
                self._aliases.setdefault(entry.path + (alias,), (header, start, end))
        for namespace in header.document.nodes('namespace_definition'):
            name = namespace.child_by_field('name')
            body = namespace.child_by_field('body')
            if name is None or body is None:
                continue
            scope = _scope_path(namespace) + (name.text.strip(),)
            names, aliases = _direct_declarations(header.tokens_in(body.span.start, body.span.end))
            self._namespace_types.setdefault(scope, set()).update(names)
            for alias, (start, end) in aliases.items():
                self._aliases.setdefault(scope + (alias,), (header, start, end))

    def resolve(self, spelling: str, scope: Tuple[str, ...] = (), _depth: int = 0) -> Optional[ClassEntry]:
        """Find the class a type spelling names, searching outward from ``scope``."""
        names = _strip_type(spelling)
        if not names or _depth > 8:
            return None
        # The outermost name is the most specific filter (e.g. BMI088 in
        # BMI088::Param); widen to the innermost name only when that fails.
        self._ensure(names[0])
        found = self._lookup(names, scope, _depth)
        if found is None and len(names) > 1:
            self._ensure(names[-1])
            found = self._lookup(names, scope, _depth)
        return found

    def _lookup(self, names, scope, _depth):
        for i in range(len(scope), -1, -1):
            path = tuple(scope[:i]) + names
            entries = self._entries.get(path)
            if entries:
                headers = sorted({str(e.header) for e in entries})
                if len(headers) > 1:
                    raise ValueError('Type %s is defined in several Module headers: %s'
                                     % ('::'.join(path), ', '.join(headers)))
                return entries[0]
            alias = self._aliases.get(path)
            if alias:
                header, start, end = alias
                return self.resolve(header.text_of(start, end).strip(), path[:-1], _depth + 1)
        return None

    def qualify_in(self, spelling: str, entry: ClassEntry, entry_spelled: str) -> str:
        """Qualify names in ``spelling`` declared in ``entry``'s class/namespace chain.

        ``entry_spelled`` is how generated code names ``entry`` (it may carry the
        enclosing template arguments, e.g. ``Outer<T>::Param``); enclosing classes
        reuse its prefix and enclosing namespaces use their plain path.
        """
        items = code_tokens(spelling)
        scopes = []  # (declared names, spelled prefix), innermost first
        spelled, path = entry_spelled, entry.path
        while path:
            classes = self._entries.get(path)
            if classes:
                scopes.append((set(classes[0].member_types()), spelled))
            else:
                scopes.append((self._namespace_types.get(path, set()), '::'.join(path)))
            path = path[:-1]
            spelled = _outer_spelling(spelled) if spelled else ''
            if not spelled:
                spelled = '::'.join(path)
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

        Returns None when a public base cannot be located, so callers can report
        it instead of silently skipping the call.
        """
        if entry.public_member_function('OnMonitor'):
            return True
        if _depth > 8:
            return None
        unknown = False
        for access, base in entry.base_spellings():
            if access != 'public':
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
