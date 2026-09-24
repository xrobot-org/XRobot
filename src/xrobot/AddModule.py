"""Add a source dependency or an ordered, editable Module instance."""
import argparse
import io
import json
import re
from pathlib import Path
import yaml
from ruamel.yaml import YAML
from xrobot.ModuleParser import discover_modules, select_module, source_interface
from xrobot.GenerateMain import load_config, validate_config, atomic_write
from xrobot.ConstructorModel import initial_arguments, template_bindings

DEFAULT_REPO_CONFIG = Path('Modules/modules.yaml')
DEFAULT_INSTANCE_CONFIG = Path('User/xrobot.yaml')
MODULES_DIR = Path('Modules')


def _round_trip():
    """YAML reader/writer that keeps comments, key order and existing quoting."""
    document = YAML()
    document.preserve_quotes = True
    document.width = 4096
    document.indent(mapping=2, sequence=4, offset=2)
    return document


def _load_round_trip(path, default):
    if not Path(path).exists():
        return default
    data = _round_trip().load(Path(path).read_text(encoding='utf-8-sig'))
    return data if data is not None else default


def _dump_round_trip(path, data):
    stream = io.StringIO()
    _round_trip().dump(data, stream)
    atomic_write(Path(path), stream.getvalue())


def is_repo_id(value):
    return '/' in value or '@' in value


def get_next_instance_id(modules, base_name):
    prefix = base_name.lower() + '_'
    names = {entry.get('id', '') for entry in modules}
    number = 0
    while prefix + str(number) in names:
        number += 1
    return prefix + str(number)


def append_module_instance(module_name, config_path=DEFAULT_INSTANCE_CONFIG, instance_id=None, modules_dir=MODULES_DIR):
    config_path = Path(config_path)
    available = discover_modules(Path(modules_dir))
    module = select_module(available, module_name)
    if not module['manifest'].standalone:
        raise ValueError('%s is a library dependency, not a Module instance' % module['id'])
    interface = source_interface(module['header'])
    print('Interface from %s' % module['header'])
    if interface['template'] is not None:
        print('template <%s>' % interface['template'])
    for item in interface['constructors']:
        print('  ' + item['declaration'])
    if config_path.exists():
        load_config(config_path)  # validate the existing file before editing it
    config = _load_round_trip(config_path, {'modules': [], 'settings': {'monitor_sleep_ms': 1000}})
    if config.get('modules') is None:
        config['modules'] = []
    identity = instance_id or get_next_instance_id(config['modules'], module['name'])
    if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', identity):
        raise ValueError('Invalid instance identifier: ' + identity)
    entry = {'module': module_name, 'id': identity}
    template_values = [p['default'] for p in interface['template_parameters']]
    if template_values:
        entry['template_args'] = template_values
    # Required template arguments stay visibly unfilled until the user selects them.
    if all(v is not None for v in template_values):
        templates = template_bindings(interface, template_values)
        cpp_class = module['name'] + ('<' + ', '.join(template_values) + '>' if template_values else '')
    else:
        templates, cpp_class = {}, module['name']
    diagnostics = []
    arguments = initial_arguments(interface, cpp_class, templates, diagnostics)
    for message in diagnostics:
        print('Unfilled default: ' + message)
    if arguments:
        entry['args'] = arguments
    config['modules'].append(entry)
    validate_config(config)
    _dump_round_trip(config_path, config)
    print('Added %s. Review the named values and fill null entries in %s.' % (identity, config_path))
    return identity


def _instance_index(config, instance_id):
    for index, entry in enumerate(config.get('modules') or []):
        if entry.get('id') == instance_id:
            return index
    raise ValueError('No module instance with id %s' % instance_id)


def set_module_instance(instance_id, values, config_path=DEFAULT_INSTANCE_CONFIG):
    """Replace an instance's id, template_args and/or args; other entries keep their text."""
    unknown = set(values) - {'id', 'template_args', 'args'}
    if unknown:
        raise ValueError('Only id, template_args and args can be set, not ' + ', '.join(sorted(unknown)))
    config_path = Path(config_path)
    load_config(config_path)
    config = _load_round_trip(config_path, {})
    entry = config['modules'][_instance_index(config, instance_id)]
    for key in ('id', 'template_args', 'args'):
        if key not in values:
            continue
        if key != 'id' and values[key] in (None, []):
            entry.pop(key, None)
        else:
            entry[key] = values[key]
    validate_config(config)
    _dump_round_trip(config_path, config)


def remove_module_instance(instance_id, config_path=DEFAULT_INSTANCE_CONFIG):
    config_path = Path(config_path)
    load_config(config_path)
    config = _load_round_trip(config_path, {})
    del config['modules'][_instance_index(config, instance_id)]
    _dump_round_trip(config_path, config)


def add_repo_entry(repo_id, config_path=DEFAULT_REPO_CONFIG):
    from xrobot.InitModule import request
    request(repo_id, canonical=True)
    path = Path(config_path)
    data = _load_round_trip(path, {'modules': []})
    if not isinstance(data.get('modules'), list):
        raise ValueError('modules.yaml requires a list')
    if repo_id not in data['modules']:
        data['modules'].append(repo_id)
        _dump_round_trip(path, data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('target')
    parser.add_argument('-c', '--config')
    parser.add_argument('-d', '--directory', default='Modules')
    parser.add_argument('--instance', action='store_true', help='Treat a full owner/repo as an instance selection')
    parser.add_argument('--instance-id')
    args = parser.parse_args()
    try:
        if is_repo_id(args.target) and not args.instance and not args.instance_id:
            add_repo_entry(args.target, args.config or DEFAULT_REPO_CONFIG)
        else:
            append_module_instance(args.target, args.config or DEFAULT_INSTANCE_CONFIG, args.instance_id, args.directory)
    except (ValueError, OSError, yaml.YAMLError) as error:
        parser.exit(1, str(error) + '\n')


def instance_main():
    """xrobot_instance: add, set or remove one Module instance of an application config."""
    parser = argparse.ArgumentParser(description=instance_main.__doc__)
    parser.add_argument('-c', '--config', default=str(DEFAULT_INSTANCE_CONFIG))
    commands = parser.add_subparsers(dest='command', required=True)
    add = commands.add_parser('add', help='append an instance filled with the source defaults')
    add.add_argument('module')
    add.add_argument('--id')
    add.add_argument('-d', '--directory', default='Modules')
    change = commands.add_parser('set', help='replace id, template_args and/or args')
    change.add_argument('id')
    change.add_argument('values', help='JSON object, e.g. {"args": [{"led": "LED_B"}]}')
    remove = commands.add_parser('remove', help='delete an instance')
    remove.add_argument('id')
    args = parser.parse_args()
    try:
        if args.command == 'add':
            append_module_instance(args.module, args.config, args.id, args.directory)
        elif args.command == 'set':
            values = json.loads(args.values)
            if not isinstance(values, dict):
                raise ValueError('values must be a JSON object')
            set_module_instance(args.id, values, args.config)
        else:
            remove_module_instance(args.id, args.config)
    except (ValueError, OSError, yaml.YAMLError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
