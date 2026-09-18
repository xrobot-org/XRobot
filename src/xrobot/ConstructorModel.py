"""Source-declared constructor contracts and explicit initializer syntax.

Only named, explicit declarations are supported. This is not a C++ type system
or structure reflection: configuration trees come from initializer expressions.
"""
import re
from xrobot.CppSource import code_tokens, close_token, split_arguments, bind_identifiers


def parameter(declaration):
    items = code_tokens(declaration)
    split = next((t for t in items if t.text == '='), None)
    head = declaration[:split.start].strip() if split else declaration.strip()
    default = declaration[split.end:].strip() if split else None
    parts = code_tokens(head)
    if not parts or parts[-1].kind != 'identifier' or len(parts) < 2:
        raise ValueError('Constructor parameters must have explicit names: ' + declaration)
    name = parts[-1].text
    cpp_type = head[:parts[-1].start].strip()
    if name in ('const', 'volatile') or cpp_type in ('class', 'typename'):
        # Type template parameters use the same named declaration representation.
        if cpp_type not in ('class', 'typename'):
            raise ValueError('Unsupported parameter declaration: ' + declaration)
    if not cpp_type or '(' in cpp_type or '[' in cpp_type or '...' in head:
        raise ValueError('Use an explicit named type alias for this declaration: ' + declaration)
    return {'name': name, 'type': cpp_type, 'default': default, 'declaration': declaration}


def class_symbols(source, name):
    """Read visible names, not aggregate members or arbitrary template semantics."""
    ts = code_tokens(source)
    begin = next(i for i, t in enumerate(ts[:-1])
                 if t.text in ('class', 'struct') and ts[i+1].text == name)
    opening = next(i for i in range(begin+2, len(ts)) if ts[i].text == '{')
    ending = close_token(ts, opening)
    access = 'public' if ts[begin].text == 'struct' else 'private'
    symbols, aliases = {}, {}
    i = opening + 1
    while i < ending:
        t = ts[i]
        if t.text in ('public', 'protected', 'private') and ts[i+1].text == ':':
            access = t.text
            i += 2
            continue
        if t.text == 'using' and ts[i+1].kind == 'identifier' and ts[i+2].text == '=':
            end = next(j for j in range(i+3, ending) if ts[j].text == ';')
            symbols[ts[i+1].text] = access
            aliases[ts[i+1].text] = source[ts[i+2].end:ts[end].start].strip()
            i = end+1
            continue
        if t.text in ('struct', 'class', 'enum'):
            j = i+1
            if t.text == 'enum' and ts[j].text in ('class', 'struct'):
                j += 1
            if ts[j].kind == 'identifier':
                symbols[ts[j].text] = access
            while j < ending and ts[j].text not in ('{', ';'):
                j += 1
            if j < ending and ts[j].text == '{':
                end = close_token(ts, j)
                if i and ts[i-1].text == 'typedef' and ts[end+1].kind == 'identifier':
                    symbols[ts[end+1].text] = access
                # Names in unscoped enums are also visible in the class scope.
                if t.text == 'enum' and ts[i+1].text not in ('class', 'struct'):
                    body = source[ts[j].end:ts[end].start]
                    for item in split_arguments(body.rstrip().rstrip(',')):
                        ident = code_tokens(item)[0]
                        if ident.kind == 'identifier':
                            symbols[ident.text] = access
                i = end+1
                continue
        if t.text == 'static':
            j = i+1
            while j < ending and ts[j].text not in (';', '{', '='):
                if ts[j].text == '(':
                    if ts[j-1].kind == 'identifier':
                        symbols[ts[j-1].text] = access
                    break
                j += 1
            if j < ending and ts[j].text in ('=', '{') and ts[j-1].kind == 'identifier':
                symbols[ts[j-1].text] = access
        if t.text in ('{', '('):
            i = close_token(ts, i)+1
            continue
        i += 1
    return symbols, aliases


def enrich_interface(source, interface):
    symbols, aliases = class_symbols(source, interface['name'])
    interface['symbols'] = symbols
    interface['aliases'] = aliases
    for ctor in interface['constructors']:
        declarations = ctor['parameters']
        if declarations == ['void']:
            declarations = []
        ctor['arguments'] = [parameter(p) for p in declarations]
    interface['template_parameters'] = [parameter(p) for p in
        split_arguments(interface['template'])] if interface['template'] else []
    return interface


def replace_names(text, replacements):
    # Scope roots such as Mode::VALUE and T::value_type must be qualified here.
    # This is intentionally different from binding external object identifiers.
    items = code_tokens(text)
    edits = []
    for i, token in enumerate(items):
        previous = items[i-1].text if i else ''
        if (token.kind == 'identifier' and token.text in replacements and
                previous not in ('.', '->', '.*', '->*', '::')):
            edits.append((token.start, token.end, replacements[token.text]))
    for start, end, replacement in reversed(edits):
        text = text[:start] + replacement + text[end:]
    return text


def template_bindings(interface, supplied):
    parameters = interface['template_parameters']
    if len(supplied) > len(parameters):
        raise ValueError('Too many template arguments for ' + interface['name'])
    replacements = {}
    for i, p in enumerate(parameters):
        value = supplied[i] if i < len(supplied) else p['default']
        if value is None:
            raise ValueError('Template argument %s.%s must be specified' % (interface['name'], p['name']))
        replacements[p['name']] = replace_names(str(value), replacements)
    return replacements


def qualify(text, interface, cpp_class, templates=None, expand_aliases=False):
    if text is None:
        return None
    substitutions = dict(templates or {})
    for symbol, access in interface['symbols'].items():
        if access == 'public':
            substitutions[symbol] = cpp_class + '::' + symbol
    # Explicit type aliases are followed only when their text is available.
    if expand_aliases:
        for _ in range(len(interface['aliases'])+1):
            altered = False
            for symbol, value in interface['aliases'].items():
                if interface['symbols'].get(symbol) != 'public':
                    continue
                target = replace_names(value, {k: v for k, v in substitutions.items() if k != symbol})
                if substitutions.get(symbol) != target:
                    substitutions[symbol] = target
                    altered = True
            if not altered:
                break
    items = code_tokens(text)
    for i, token in enumerate(items):
        if interface['symbols'].get(token.text) not in ('private', 'protected'):
            continue
        previous = items[i-1].text if i else ''
        own_scope = previous == '::' and i >= 2 and items[i-2].text == interface['name']
        if previous not in ('.', '->', '.*', '->*', '::') or own_scope:
            raise ValueError('Expression uses non-public member %s::%s' % (interface['name'], token.text))
    return replace_names(text, substitutions)


def initializer_tree(expression, expected_type=None):
    """Expand a brace initializer's explicit entries; never inspect a type's fields."""
    if expression is None:
        return None
    ts = code_tokens(expression)
    if not ts:
        return expression
    opening = next((i for i, t in enumerate(ts) if t.text == '{'), None)
    if opening is None or close_token(ts, opening) != len(ts)-1:
        return expression
    # Do not erase an unknown conversion, a lambda, or a nested typed expression.
    # A typed outer initializer can be expanded only when its explicit type is
    # the declared parameter type; braced child values need no type reflection.
    prefix = expression[:ts[opening].start].strip()
    if prefix:
        if expected_type is None or type_shape(prefix)[:3] != type_shape(expected_type)[:3]:
            # Const on the referred-to configuration does not change its value type.
            if expected_type is None or type_shape(prefix)[0] != type_shape(expected_type)[0] or type_shape(prefix)[2] or type_shape(expected_type)[2]:
                return expression
        if any(t.text in ('[', ']', '(', ')', '=', '?', '+', '-') for t in ts[:opening]):
            return expression
    body = expression[ts[opening].end:ts[-1].start].strip().rstrip(',').strip()
    if not body:
        return []
    parts = split_arguments(body)
    fields = []
    for item in parts:
        its = code_tokens(item)
        if len(its) >= 3 and its[0].text == '.' and its[1].kind == 'identifier' and its[2].text == '=':
            fields.append((its[1].text, initializer_tree(item[its[2].end:].strip())))
        else:
            fields.append(None)
    if all(f is not None for f in fields):
        result = {}
        for name, value in fields:
            if name in result:
                raise ValueError('Duplicate initializer field: ' + name)
            result[name] = value
        return result
    if any(f is not None for f in fields):
        return expression
    return [initializer_tree(p) for p in parts]


def compliant_constructors(interface, cpp_class=None, templates=None):
    """Accept the agreed constructor shape: dependencies first, then defaults."""
    cpp_class = cpp_class or interface['name']
    accepted, rejected = [], []
    for ctor in interface['constructors']:
        config_started = False
        problems = []
        for p in ctor['arguments']:
            if p['default'] is not None:
                config_started = True
            elif config_started:
                problems.append('%s: dependency without a default appears after value configuration' % p['name'])
        if problems:
            rejected.append('line %s: %s' % (ctor.get('line', '?'), '; '.join(problems)))
        else:
            accepted.append(ctor)
    if not accepted:
        raise ValueError('%s: no compliant constructor; %s' % (
            interface['name'], ' | '.join(rejected)))
    return accepted


def initial_arguments(interface, cpp_class=None, templates=None, diagnostics=None):
    cpp_class = cpp_class or interface['name']
    result = []
    ctor = compliant_constructors(interface, cpp_class, templates)[0]
    for p in ctor['arguments']:
        try:
            value = initializer_tree(qualify(p['default'], interface, cpp_class, templates),
                                     qualify(p['type'], interface, cpp_class, templates))
        except ValueError as error:
            message = '%s.%s: %s; original default: %s' % (
                cpp_class, p['name'], error, p['default'])
            if diagnostics is None:
                raise ValueError(message) from error
            diagnostics.append(message)
            value = None
        result.append({p['name']: value})
    return result


def type_shape(cpp_type):
    """Split only outer cv/pointer/ref declarators, never template arguments."""
    text = cpp_type.strip()
    items = code_tokens(text)
    outer = []
    i = 0
    while i < len(items):
        if items[i].text in ('<', '(', '[', '{'):
            i = close_token(items, i) + 1
        else:
            outer.append(items[i])
            i += 1
    reference = ''
    if outer and outer[-1].text in ('&', '&&'):
        reference = outer[-1].text
        text = text[:outer.pop().start].rstrip()
    stars = [t for t in outer if t.text == '*']
    base_end = stars[0].start if stars else len(text)
    qualifiers = [t for t in outer if t.start < base_end and
                  t.text in ('const', 'volatile', 'typename')]
    base = text[:base_end]
    for token in reversed(qualifiers):
        base = base[:token.start] + base[token.end:]
    base = re.sub(r'\s+', '', base)
    cv = frozenset(t.text for t in qualifiers if t.text != 'typename')
    pointers = []
    for i, star in enumerate(stars):
        end = stars[i+1].start if i+1 < len(stars) else len(text)
        pointers.append(frozenset(t.text for t in outer if star.end <= t.start < end
                                 and t.text in ('const', 'volatile')))
    return base, cv, tuple(pointers), reference


def view_conversion(view_type, target_type, field):
    """Return only an explicit same-type/cv or address-of binding, else no match."""
    vb, vc, vp, _ = type_shape(view_type)
    tb, tc, tp, ref = type_shape(target_type)
    if vb != tb or not vc.issubset(tc) or ref == '&&':
        return None
    if vp == tp:
        # A reference to a pointer variable must retain pointee qualifications.
        if vp and ref == '&' and (vc != tc or vp != tp):
            return None
        return field
    if not vp and len(tp) == 1 and ref != '&':
        return 'std::addressof(%s)' % field
    # Adding top-level const to a reference to existing pointer storage is safe.
    if len(vp) == len(tp) and vp and vc == tc and vp[:-1] == tp[:-1] and vp[-1].issubset(tp[-1]):
        return field
    return None



def matching_views(options, target_type):
    candidates = []
    tb, tc, tp, _ = type_shape(target_type)
    for typ, field in options:
        bound = view_conversion(typ, target_type, field)
        if bound is None:
            continue
        vb, vc, vp, _ = type_shape(typ)
        score = (0 if vp == tp else 1, len(tc - vc))
        candidates.append((score, bound))
    if not candidates:
        return []
    best = min(score for score, _ in candidates)
    return [bound for score, bound in candidates if score == best]


def bind_value(expression, target_type, views, bindings):
    text = expression.strip()
    if text in views:
        candidates = matching_views(views[text], target_type)
        if len(candidates) == 1:
            return candidates[0]
        if len(candidates) > 1:
            raise ValueError('Ambiguous registered name %s for %s' % (text, target_type))
        # A sole explicit view can still participate in conversions checked by C++.
        # Multiple views must never silently select an arbitrary first entry.
        if len(views[text]) == 1:
            view_type, field = views[text][0]
            _, _, source_pointer, _ = type_shape(view_type)
            _, _, target_pointer, reference = type_shape(target_type)
            if not source_pointer and len(target_pointer) == 1 and reference != '&':
                return 'std::addressof(%s)' % field
            return field
        raise ValueError('No unique registered view of %s for %s' % (text, target_type))
    return bind_identifiers(expression, bindings)



def explicit_expression_type(value):
    """Recognize explicit casts/initializers and a small portable literal subset."""
    if isinstance(value, bool):
        return 'bool'
    if isinstance(value, int):
        return 'int' if -32767 <= value <= 32767 else None
    if isinstance(value, float):
        return 'double'
    if not isinstance(value, str):
        return None
    value = value.strip()
    if value in ('true', 'false'):
        return 'bool'
    ts = code_tokens(value)
    if len(ts) >= 5 and ts[0].text in ('static_cast', 'const_cast', 'reinterpret_cast', 'dynamic_cast') and ts[1].text == '<':
        ending = close_token(ts, 1)
        if ending+1 < len(ts) and ts[ending+1].text == '(' and close_token(ts, ending+1) == len(ts)-1:
            return value[ts[1].end:ts[ending].start]
    opening = next((i for i, t in enumerate(ts) if t.text == '{'), None)
    if opening and close_token(ts, opening) == len(ts)-1:
        prefix = value[:ts[opening].start].strip()
        if re.fullmatch(r'[A-Za-z_][A-Za-z_0-9:]*(?:\s*<[^{};]+>)?', prefix):
            return prefix
    if re.fullmatch(r'[+-]?\d+', value) and -32767 <= int(value) <= 32767:
        return 'int'
    if re.fullmatch(r'[+-]?(?:\d+\.\d*|\d*\.\d+|\d+[eE][+-]?\d+)(?:[eE][+-]?\d+)?[fFlL]?', value):
        return 'float' if value[-1:] in ('f', 'F') else 'long double' if value[-1:] in ('l', 'L') else 'double'
    return None


def constructor_for(interface, named_values, views, cpp_class, templates):
    names = [next(iter(v)) for v in named_values]
    candidates = []
    supported = compliant_constructors(interface, cpp_class, templates)
    for ctor in supported:
        if [p['name'] for p in ctor['arguments']] != names:
            continue
        good = True
        for p, item in zip(ctor['arguments'], named_values):
            value = next(iter(item.values()))
            if isinstance(value, str) and value.strip() in views:
                typ = qualify(p['type'], interface, cpp_class, templates, True)
                options = views[value.strip()]
                if not any(view_conversion(t, typ, f) is not None for t, f in options):
                    # Unknown conversions are not a reason to reject the only signature.
                    good = False
            else:
                known = explicit_expression_type(value)
                if known is not None:
                    target = qualify(p['type'], interface, cpp_class, templates, True)
                    source = qualify(known, interface, cpp_class, templates, True)
                    if type_shape(source)[0] != type_shape(target)[0] or type_shape(source)[2] != type_shape(target)[2]:
                        good = False
        candidates.append((ctor, good))
    if len(candidates) == 1:
        return candidates[0][0]
    typed = [ctor for ctor, good in candidates if good]
    if len(typed) == 1:
        return typed[0]
    if not candidates:
        raise ValueError('%s: named arguments %s do not match any constructor; expected %s' % (
            interface['name'], ', '.join(names), ' | '.join(', '.join(p['name'] for p in c['arguments']) for c in supported)))
    raise ValueError('%s: constructor is ambiguous for the supplied names and explicit types' % interface['name'])


def scalar_text(value, field):
    if value is None:
        raise ValueError(field + ' is not filled in')
    if isinstance(value, bool):
        return 'true' if value else 'false'
    if isinstance(value, (int, float)):
        return str(value)
    if not isinstance(value, str) or not value.strip():
        raise ValueError(field + ' requires C++ expression text')
    if value.lstrip().startswith('@'):
        raise ValueError(field + ': use ordinary C++ expressions, not @ syntax')
    return value


def render_value(value, field, bindings):
    if isinstance(value, dict):
        entries = []
        for key, child in value.items():
            if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', str(key)):
                raise ValueError(field + ': invalid aggregate field ' + str(key))
            entries.append('.%s = %s' % (key, render_value(child, field+'.'+key, bindings)))
        return '{\n' + '\n, '.join(entries) + '\n}' if entries else '{}'
    if isinstance(value, list):
        return '{\n' + '\n, '.join(render_value(v, field+'[%d]' % i, bindings) for i, v in enumerate(value)) + '\n}' if value else '{}'
    return bind_identifiers(scalar_text(value, field), bindings)


def construct_arguments(interface, named_values, cpp_class, templates, views,
                        bindings, identity, compile_check=False):
    ctor = constructor_for(interface, named_values, views, cpp_class, templates)
    pin_types = len(compliant_constructors(interface, cpp_class, templates)) > 1
    declarations, arguments = [], []
    for p, item in zip(ctor['arguments'], named_values):
        value = next(iter(item.values()))
        field = identity+'.args.'+p['name']
        typ = qualify(p['type'], interface, cpp_class, templates)
        target = qualify(p['type'], interface, cpp_class, templates, True)
        if value is None and compile_check and p['default'] is None:
            raw = '*static_cast<std::remove_reference_t<%s>*>(xr_ci_null)' % typ
            expr = 'static_cast<%s>(%s)' % (typ, raw) if typ.rstrip().endswith('&&') else raw
        elif isinstance(value, str):
            expr = bind_value(scalar_text(value, field), target, views, bindings)
        else:
            expr = render_value(value, field, bindings)
        # A named dependency already has caller-owned lifetime. Passing its bound
        # view directly avoids a second static reference slot and its guard, while
        # the cast keeps the selected overload's exact reference/cv semantics.
        reference = type_shape(target)[3]
        if reference and isinstance(value, str) and value.strip() in views:
            arguments.append('static_cast<%s>(%s)' % (typ, expr))
            continue
        # Config temporaries and initializer-list backing arrays still require
        # lifetime extension. Do not remove storage merely because a type is const.
        if reference or 'std::initializer_list<' in target.replace(' ', ''):
            storage = 'xr_arg_%s_%s' % (identity, p['name'])
            declarations.append('  static %s %s =\n      %s\n  ;' % (typ, storage, expr))
            arguments.append('static_cast<%s>(%s)' % (typ, storage) if typ.rstrip().endswith('&&') else storage)
        elif isinstance(value, (dict, list)) or expr.lstrip().startswith('{'):
            arguments.append('%s%s' % (typ, expr))
        elif pin_types:
            # C++ cannot see YAML parameter names. Preserve the chosen overload's
            # value types without a forwarding helper that would break prvalue
            # copy elision for immovable configurations. The check still rejects
            # explicit-only conversions that an ordinary argument would reject.
            expression_type = 'decltype((\n%s\n))' % expr
            declarations.append(
                '  static_assert(std::is_convertible<%s, %s>::value ||\n'
                '                std::is_same<std::remove_cv_t<%s>, std::remove_cv_t<%s>>::value,\n'
                '                "Named constructor argument requires an implicit conversion");' %
                (expression_type, typ, expression_type, typ))
            arguments.append('static_cast<%s>(\n%s\n)' % (typ, expr))
        else:
            arguments.append(expr)
    return declarations, arguments
