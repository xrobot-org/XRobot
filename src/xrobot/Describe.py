"""Describe a BSP's XRobot state as JSON for editors; reads only, never writes.

The output is what xrobot_gen_main reads and enforces: locked Module sources,
constructor signatures and the struct fields / constructors a YAML mapping must
name (the same TypeIndex lookups), XR_REGISTER registrations with their types,
lock consistency, entry-header stamp freshness and generation diagnostics.
Editors render and edit from it without parsing C++ or manifests themselves.
"""
import argparse
import json
import sys
from pathlib import Path
import yaml

from xrobot.GenerateMain import (load_config, read_registrations, generate_xrobot_main_code,
                                 stamp_state)
from xrobot.ModuleParser import (discover_modules, locked_modules, lock_error, _module_record,
                                 select_module, source_interface)
from xrobot.ConstructorModel import (compliant_constructors, initializer_tree, qualify,
                                     matching_views, type_shape)
from xrobot.TypeIndex import TypeIndex

SCHEMA = 1


def _relative(path, root):
    try:
        return Path(path).resolve().relative_to(Path(root).resolve()).as_posix()
    except ValueError:
        return Path(path).as_posix()


class _Types:
    """Collect the mapping shape of every type reachable from a constructor parameter."""

    def __init__(self, index):
        self.index = index
        self.table = {}

    def ref(self, cpp_type, scope=()):
        try:
            entry = self.index.resolve(cpp_type, scope) if cpp_type else None
        except ValueError:
            return None
        if entry is None:
            return None
        key = entry.qualified
        if key not in self.table:
            self.table[key] = None  # reserve before recursing into member types
            if entry.is_aggregate():
                self.table[key] = {'kind': 'aggregate', 'fields': [
                    self._member(name, self.index.qualify_in(typ, entry, key), entry)
                    for name, typ, _ in entry.fields()]}
            else:
                self.table[key] = {'kind': 'class', 'constructors': [
                    [dict(self._member(p['name'], self.index.qualify_in(p['type'], entry, key), entry),
                          default=p['default']) for p in ctor]
                    for ctor in entry.constructors() if ctor]}
        return key

    def _member(self, name, cpp_type, entry):
        return {'name': name, 'type': cpp_type, 'type_ref': self.ref(cpp_type, entry.path)}


def _is_a(index, class_name, base, depth=0):
    """Whether ``class_name`` is ``base`` or publicly derives from it (loaded headers only)."""
    if class_name == base or class_name.split('::')[-1] == base.split('::')[-1]:
        return True
    try:
        entry = index.resolve(class_name) if depth < 8 else None
    except ValueError:
        return False
    return entry is not None and any(
        access == 'public' and _is_a(index, type_shape(spelling)[0].split('<')[0], base, depth + 1)
        for access, spelling in entry.base_spellings())


def _candidates(target, index, registrations, instances):
    """Registered names and earlier-configurable instance ids a parameter can bind."""
    names = [r['name'] for r in registrations
             if matching_views([(t, r['name']) for t in r['types']], target)]
    base = type_shape(target)[0].split('<')[0]
    names += [i['id'] for i in instances if i['class'] and _is_a(index, i['class'], base)]
    return names


def _describe_module(module, root, types, registrations, instances):
    result = {'id': module['id'], 'class': module['name'],
              'header': _relative(module['header'], root),
              'standalone': bool(module['manifest'].standalone)}
    if not result['standalone']:
        return result
    interface = source_interface(module['header'])
    cpp_class = module['name']
    result['template_parameters'] = [
        {'name': p['name'], 'type': p['type'], 'default': p['default']}
        for p in interface['template_parameters']]
    constructors = []
    for ctor in compliant_constructors(interface, cpp_class):
        parameters = []
        for p in ctor['arguments']:
            target = qualify(p['type'], interface, cpp_class, None, True)
            default = qualify(p['default'], interface, cpp_class)
            try:
                tree = initializer_tree(default, target)
            except ValueError:
                tree = default
            parameters.append({
                'name': p['name'], 'type': target, 'default': default,
                # A designated default initializer fixes the mapping's fields.
                'default_fields': tree if isinstance(tree, dict) else None,
                'type_ref': types.ref(target),
                'candidates': _candidates(target, types.index, registrations, instances),
            })
        constructors.append({'line': ctor.get('line'), 'parameters': parameters})
    result['constructors'] = constructors
    return result


def describe(project='.', config=None, register_sources=None, lock=None, output=None):
    root = Path(project)
    config_path = Path(config) if config else root / 'User/xrobot.yaml'
    lock_path = Path(lock) if lock else root / 'xrobot.lock'
    header = Path(output) if output else root / 'User/xrobot_main.hpp'
    modules_dir = root / 'Modules'
    diagnostics = []

    def report(severity, scope, message):
        diagnostics.append({'severity': severity, 'scope': scope, 'message': str(message)})

    # Lock: every entry must be checked out at its commit; only locked sources are read.
    lock_info = {'path': _relative(lock_path, root), 'present': lock_path.is_file(), 'modules': []}
    modules = {}
    if lock_path.is_file():
        for state in locked_modules(modules_dir, lock_path):
            lock_info['modules'].append({k: state[k] for k in ('id', 'commit', 'head', 'status')})
            if state['status'] == 'ok':
                modules[state['id']] = _module_record(state['id'], state['folder'])
            else:
                report('error', state['id'], lock_error(state))
        statuses = {m['status'] for m in lock_info['modules']}
        lock_info['status'] = 'ok' if statuses <= {'ok'} else 'missing' if 'missing' in statuses else 'mismatch'
    else:
        lock_info['status'] = 'absent'
        try:
            modules = discover_modules(modules_dir, lock_path)
        except ValueError as error:
            report('error', 'Modules', error)

    entry = dict(stamp_state(header), path=_relative(header, root))
    if entry['status'] == 'stale':
        changed = ', '.join(i['path'] for i in entry['inputs'] if i['status'] != 'fresh')
        report('warning', entry['path'], 'generated from older %s; regenerate it' % changed)

    sources = [Path(p) for p in (register_sources or [])]
    if not sources and (config_path.parent / 'app_main.cpp').is_file():
        sources = [config_path.parent / 'app_main.cpp']
    records = []
    try:
        records = read_registrations(sources)
    except (OSError, ValueError) as error:
        report('error', 'registrations', error)
    registrations = [{'name': r['name'], 'types': list(r['types'])} for r in records]

    config_data, instances = None, []
    try:
        config_data = load_config(config_path)
    except (OSError, ValueError, yaml.YAMLError) as error:
        report('error', _relative(config_path, root), error)
    for item in (config_data or {}).get('modules', []):
        try:
            module = select_module(modules, item['module'])
        except ValueError:
            module = None  # reported by the generation diagnostic below
        instances.append({'id': item['id'], 'module': item['module'],
                          'class': module['name'] if module else None,
                          'template_args': item.get('template_args', []),
                          'args': item.get('args', [])})

    types = _Types(TypeIndex.for_modules(modules))
    described = {}
    for identity, module in sorted(modules.items()):
        try:
            described[identity] = _describe_module(module, root, types, registrations, instances)
        except (OSError, ValueError) as error:
            described[identity] = {'id': identity, 'class': module['name'], 'error': str(error)}
            report('error', identity, error)

    if config_data is not None and lock_info['status'] in ('ok', 'absent'):
        try:
            generate_xrobot_main_code(config_data, modules, records)
        except (OSError, ValueError) as error:
            report('error', _relative(config_path, root), error)

    return {
        'schema': SCHEMA,
        'config': _relative(config_path, root),
        'lock': lock_info,
        'entry': entry,
        'registrations': registrations,
        'modules': described,
        'types': types.table,
        'instances': instances,
        'diagnostics': diagnostics,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-C', '--project', default='.', help='BSP root (contains Modules/ and xrobot.lock)')
    parser.add_argument('-c', '--config', help='default: <project>/User/xrobot.yaml')
    parser.add_argument('-o', '--output', help='generated entry; default: <project>/User/xrobot_main.hpp')
    parser.add_argument('--register-source', action='append', default=[])
    parser.add_argument('--lock', help='default: <project>/xrobot.lock')
    args = parser.parse_args()
    try:
        result = describe(args.project, args.config, args.register_source, args.lock, args.output)
    except (OSError, ValueError, yaml.YAMLError) as error:
        parser.exit(1, str(error) + '\n')
    json.dump(result, sys.stdout, indent=2, ensure_ascii=False, default=str)
    sys.stdout.write('\n')


if __name__ == '__main__':
    main()
