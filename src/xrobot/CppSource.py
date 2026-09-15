"""Small source-text lexer used for declarations and registered-name binding.

This module never resolves C++ types or evaluates expressions. Tokens retain
source offsets so diagnostics and unmodified expression text remain intact.
"""
from dataclasses import dataclass
import re
from typing import Dict, List


@dataclass(frozen=True)
class Token:
    text: str
    start: int
    end: int
    kind: str


_IDENTIFIER = re.compile(r'[A-Za-z_][A-Za-z_0-9]*')
_NUMBER = re.compile(r"(?:\d|\.\d)[A-Za-z_0-9.'+-]*")
_RAW = re.compile(r'(?:u8|u|U|L)?R"([^ ()\\\t\r\n]{0,16})\(')
_STRING = re.compile(r'(?:u8|u|U|L)?[\"\']')


def tokens(source: str) -> List[Token]:
    result = []
    i = 0
    while i < len(source):
        start = i
        if source[i].isspace():
            i += 1
            continue
        if source.startswith('//', i):
            end = source.find('\n', i)
            i = len(source) if end < 0 else end
            result.append(Token(source[start:i], start, i, 'comment'))
            continue
        if source.startswith('/*', i):
            end = source.find('*/', i + 2)
            if end < 0:
                raise ValueError('Unterminated C++ comment')
            i = end + 2
            result.append(Token(source[start:i], start, i, 'comment'))
            continue
        match = _RAW.match(source, i)
        if match:
            close = ')' + match.group(1) + '"'
            end = source.find(close, match.end())
            if end < 0:
                raise ValueError('Unterminated C++ raw string')
            i = end + len(close)
            suffix = _IDENTIFIER.match(source, i)
            if suffix:
                i = suffix.end()
            result.append(Token(source[start:i], start, i, 'literal'))
            continue
        match = _STRING.match(source, i)
        if match:
            quote = source[match.end() - 1]
            i = match.end()
            while i < len(source):
                if source[i] == '\\':
                    i += 2
                elif source[i] == quote:
                    i += 1
                    break
                else:
                    i += 1
            else:
                raise ValueError('Unterminated C++ string or character literal')
            suffix = _IDENTIFIER.match(source, i)
            if suffix:
                i = suffix.end()
            result.append(Token(source[start:i], start, i, 'literal'))
            continue
        if source[i] == '#' and not source[source.rfind('\n', 0, i) + 1:i].strip():
            i = source.find('\n', i)
            if i < 0:
                i = len(source)
            while i < len(source) and source[start:i].rstrip('\r').endswith('\\'):
                nxt = source.find('\n', i + 1)
                i = len(source) if nxt < 0 else nxt
            result.append(Token(source[start:i], start, i, 'directive'))
            continue
        match = _IDENTIFIER.match(source, i)
        if match:
            i = match.end()
            result.append(Token(source[start:i], start, i, 'identifier'))
            continue
        # Keep pp-numbers together without swallowing binary + or - operators.
        if source[i].isdigit() or (source[i] == '.' and i + 1 < len(source) and source[i+1].isdigit()):
            i += 1
            while i < len(source):
                if source[i].isalnum() or source[i] in "_.'":
                    i += 1
                elif source[i] in '+-' and source[i-1] in 'eEpP':
                    i += 1
                else:
                    break
            result.append(Token(source[start:i], start, i, 'number'))
            continue
        punct = next((p for p in ('::', '->*', '->', '.*', '...', '&&', '||', '==', '!=', '<=', '>=', '++', '--') if source.startswith(p, i)), source[i])
        i += len(punct)
        result.append(Token(punct, start, i, 'punct'))
    return result


def code_tokens(source: str) -> List[Token]:
    return [t for t in tokens(source) if t.kind not in ('comment', 'directive')]


def close_token(items: List[Token], start: int) -> int:
    pairs = {'(': ')', '[': ']', '{': '}', '<': '>'}
    expected = pairs.get(items[start].text)
    if expected is None:
        raise ValueError('Expected opening delimiter')
    stack = [expected]
    for i in range(start + 1, len(items)):
        text = items[i].text
        if items[i].kind == 'literal':
            continue
        if text in ('(', '[', '{'):
            stack.append(pairs[text])
        elif text == '<' and stack[-1] == '>':
            stack.append('>')
        elif text == stack[-1]:
            stack.pop()
            if not stack:
                return i
    raise ValueError('Unclosed delimiter at offset %d' % items[start].start)


def split_arguments(text: str) -> List[str]:
    if not text.strip():
        return []
    items = code_tokens(text)
    parts, start, stack = [], 0, []
    pairs = {'(': ')', '[': ']', '{': '}', '<': '>'}
    for token in items:
        symbol = token.text
        if token.kind == 'literal':
            continue
        if symbol in ('(', '[', '{'):
            stack.append(pairs[symbol])
        elif symbol == '<' and (not stack or stack[-1] == '>'):
            stack.append('>')
        elif stack and symbol == stack[-1]:
            stack.pop()
        elif symbol == ',' and not stack:
            parts.append(text[start:token.start].strip())
            start = token.end
    if stack:
        raise ValueError('Unbalanced argument list; parenthesize comparison expressions')
    parts.append(text[start:].strip())
    if any(not p for p in parts):
        raise ValueError('Empty argument in C++ declaration')
    return parts


def bind_identifiers(expression: str, bindings: Dict[str, List[str]]) -> str:
    """Bind unqualified logical identifiers, preserving all other source bytes."""
    items = tokens(expression)
    edits = []
    significant = [t for t in items if t.kind not in ('comment', 'directive')]
    for i, token in enumerate(significant):
        if token.kind != 'identifier' or token.text not in bindings:
            continue
        prev = significant[i-1].text if i else ''
        nxt = significant[i+1].text if i+1 < len(significant) else ''
        if prev in ('.', '->', '.*', '->*', '::') or nxt == '::':
            continue
        views = bindings[token.text]
        if len(views) != 1:
            raise ValueError("Ambiguous registered name '%s'; select one of: %s" % (token.text, ', '.join(views)))
        edits.append((token.start, token.end, views[0]))
    for start, end, replacement in reversed(edits):
        expression = expression[:start] + replacement + expression[end:]
    return expression


def extract_interface(source: str, name: str) -> dict:
    """Extract an explicit global class and public constructor declaration text."""
    items = code_tokens(source)
    brace_depth = 0
    found = None
    for i, token in enumerate(items[:-1]):
        if token.text == '{':
            brace_depth += 1
        elif token.text == '}':
            brace_depth -= 1
        elif token.text in ('class', 'struct') and items[i+1].text == name:
            if i and items[i-1].text == 'enum':
                continue
            j = i + 2
            while j < len(items) and items[j].text not in (';', '{'):
                j += 1
            if j == len(items) or items[j].text == ';':
                continue
            if brace_depth:
                raise ValueError('%s must be an explicitly declared global Module class' % name)
            if found is not None:
                raise ValueError('Multiple definitions of Module class %s' % name)
            found = (i, j, close_token(items, j))
    if found is None:
        raise ValueError('No explicit global class %s; macro-generated interfaces are not supported' % name)
    begin, opening, ending = found
    template = None
    if begin and items[begin-1].text == '>':
        for i in range(begin-1, -1, -1):
            if items[i].text == 'template' and i+1 < begin and items[i+1].text == '<':
                end = close_token(items, i+1)
                if end != begin-1:
                    break
                template = source[items[i+1].end:items[end].start].strip()
                if not template:
                    raise ValueError('Explicit Module template specialization is not supported')
                break
    access = 'public' if items[begin].text == 'struct' else 'private'
    constructors = []
    depth = 0
    i = opening + 1
    while i < ending:
        token = items[i]
        if token.text == '{':
            i = close_token(items, i) + 1
            continue
        if token.text in ('public', 'private', 'protected') and i+1 < ending and items[i+1].text == ':':
            access = token.text
            i += 2
            continue
        if token.text == name and i+1 < ending and items[i+1].text == '(' and (i == 0 or items[i-1].text not in ('~', '::', '.', '->', 'new')):
            end = close_token(items, i+1)
            if access == 'public':
                prefix = source[items[opening].end:token.start]
                # Preprocessor branches around public declarations are not an interface model.
                level = 0
                for line in prefix.splitlines():
                    if re.match(r'^\s*#\s*(if|ifdef|ifndef)\b', line):
                        level += 1
                    elif re.match(r'^\s*#\s*endif\b', line):
                        level -= 1
                if level:
                    raise ValueError('%s constructor interface varies under #if' % name)
                text = source[token.start:items[end].end]
                if end+2 < len(items) and items[end+1].text == '=' and items[end+2].text == 'delete':
                    i = end + 3
                    continue
                constructors.append({'declaration': text, 'parameters': split_arguments(source[items[i+1].end:items[end].start]), 'line': source.count('\n', 0, token.start)+1})
            # Skip the initializer list and body without treating calls there as constructors.
            j = end + 1
            while j < ending:
                if items[j].text == ';':
                    j += 1
                    break
                if items[j].text == '(':
                    j = close_token(items, j) + 1
                    continue
                if items[j].text == '{':
                    stop = close_token(items, j)
                    if stop+1 < ending and items[stop+1].text in (',', '{'):
                        j = stop + 1
                        continue
                    j = stop + 1
                    break
                j += 1
            i = j
            continue
        i += 1
    if not constructors:
        raise ValueError('No supported explicit public constructor for %s' % name)
    return {'name': name, 'template': template, 'constructors': constructors}
