"""Fetch sources and generate C++; configure/build/test remain native BSP steps."""
import argparse
from pathlib import Path
import yaml
from xrobot.InitModule import sync_modules_by_config
from xrobot.GenerateMain import generate, atomic_write

MODULES_YAML_TEMPLATE = 'modules:\n  - xrobot-org/BlinkLED\n'
SOURCES_YAML_TEMPLATE = 'sources:\n  - url: https://xrobot.work/xrobot-modules/index.yaml\n    priority: 0\n'


def setup(project=Path('.'), config=None, update=False, frozen=False, offline=False, register_sources=None):
    project = Path(project)
    modules = project / 'Modules'
    created = []
    for path, text in ((modules / 'modules.yaml', MODULES_YAML_TEMPLATE), (modules / 'sources.yaml', SOURCES_YAML_TEMPLATE), (project / 'User/xrobot.yaml', 'modules: []\nsettings:\n  monitor_sleep_ms: 1000\n')):
        if not path.exists():
            atomic_write(path, text)
            created.append(path)
    if created:
        print('Created %s. Select sources and instances before running setup again.' % ', '.join(map(str, created)))
        return False
    sync_modules_by_config(modules / 'modules.yaml', modules / 'sources.yaml', modules, project / 'xrobot.lock', update, frozen, offline)
    config_path = Path(config) if config else project / 'User/xrobot.yaml'
    generate(config_path, modules, project / 'User/xrobot_main.hpp', register_sources, project / 'xrobot.lock')
    print('Generated User/xrobot_main.hpp. Build/test with your BSP CMake or native tooling.')
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-d', '--directory', default='.')
    parser.add_argument('-c', '--config')
    parser.add_argument('--register-source', action='append', default=[])
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--update', action='store_true')
    group.add_argument('--frozen', action='store_true')
    parser.add_argument('--offline', action='store_true')
    args = parser.parse_args()
    try:
        setup(args.directory, args.config, args.update, args.frozen, args.offline, args.register_source)
    except (OSError, ValueError, yaml.YAMLError) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
