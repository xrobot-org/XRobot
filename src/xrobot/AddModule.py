"""Add a source dependency or an ordered, editable Module instance."""
import argparse
import re
from pathlib import Path
import yaml
from xrobot.ModuleParser import discover_modules, select_module, source_interface
from xrobot.GenerateMain import load_config, validate_config, atomic_write
from xrobot.SourceManager import load_yaml

DEFAULT_REPO_CONFIG = Path('Modules/modules.yaml')
DEFAULT_INSTANCE_CONFIG = Path('User/xrobot.yaml')
MODULES_DIR = Path('Modules')


def is_repo_id(value):
    return '/' in value or '@' in value


def get_next_instance_id(modules, base_name):
    prefix = base_name.lower()
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
    config = load_config(config_path) if config_path.exists() else {'modules': [], 'settings': {'monitor_sleep_ms': 1000}}
    config.setdefault('modules', [])
    identity = instance_id or get_next_instance_id(config['modules'], module['name'])
    if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', identity):
        raise ValueError('Invalid instance identifier: ' + identity)
    config['modules'].append({'module': module_name, 'id': identity})
    validate_config(config)
    atomic_write(config_path, yaml.safe_dump(config, sort_keys=False, allow_unicode=True))
    print('Added %s. Set ordered args in %s; omitted defaults remain in C++.' % (identity, config_path))
    return identity


def add_repo_entry(repo_id, config_path=DEFAULT_REPO_CONFIG):
    from xrobot.InitModule import request
    request(repo_id, canonical=True)
    path = Path(config_path)
    data = load_yaml(path) if path.exists() else {'modules': []}
    if not isinstance(data.get('modules'), list):
        raise ValueError('modules.yaml requires a list')
    if repo_id not in data['modules']:
        data['modules'].append(repo_id)
        atomic_write(path, yaml.safe_dump(data, sort_keys=False, allow_unicode=True))


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


if __name__ == '__main__':
    main()
