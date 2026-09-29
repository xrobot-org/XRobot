"""xrobot: resolve Modules, generate the static entry and edit configurations.

Run from anywhere inside a BSP (the directory tree containing
Modules/modules.yaml); ``-C DIR`` starts the search at DIR instead.
"""
import argparse
import json
import sys
from pathlib import Path

import yaml

from xrobot import __version__
from xrobot.config import ConfigError
from xrobot.project import Project, ProjectError, find_root

INIT_MODULES = 'xrobot: %s\nmodules: []\n'
INIT_SOURCES = ('sources:\n'
                '  - url: https://xrobot.work/xrobot-modules/index.yaml\n'
                '    priority: 0\n')
INIT_CONFIG = 'modules: []\nsettings:\n  monitor_sleep_ms: 1000\n'


def _project(args):
    return Project(find_root(args.directory))


def _config_path(project, value):
    if value is None:
        return None
    path = Path(value)
    if not path.is_absolute():
        candidate = Path.cwd() / path
        path = candidate if candidate.exists() else project.root / path
    return path.resolve()


def _count(number, noun):
    return '%d %s%s' % (number, noun, '' if number == 1 else 's')


def _pin_warning(project):
    from xrobot.init_module import read_modules_yaml
    _, pin = read_modules_yaml(project.modules_yaml)
    if pin is None:
        print('warning: Modules/modules.yaml does not pin XRobot; add `xrobot: %s`' % __version__,
              file=sys.stderr)
    elif pin != __version__:
        print('warning: installed XRobot %s differs from the pinned %s' % (__version__, pin),
              file=sys.stderr)


def cmd_init(args):
    root = Path(args.directory).resolve()
    from xrobot.generate_main import atomic_write
    created = []
    for relative, text in (('Modules/modules.yaml', INIT_MODULES % __version__),
                           ('Modules/sources.yaml', INIT_SOURCES),
                           ('User/xrobot.yaml', INIT_CONFIG)):
        path = root / relative
        if not path.exists():
            atomic_write(path, text)
            created.append(relative)
    ignore = root / '.gitignore'
    lines = ignore.read_text(encoding='utf-8').splitlines() if ignore.exists() else []
    wanted = ['/User/xrobot_main.hpp', '/Modules/CMakeLists.txt', '/Modules/*/']
    missing = [line for line in wanted if line not in lines]
    if missing:
        atomic_write(ignore, '\n'.join(lines + missing) + '\n')
        created.append('.gitignore entries')
    print('Created %s' % (', '.join(created) if created else 'nothing (already initialized)'))


def cmd_setup(args):
    from xrobot.init_module import sync_modules, check_tool_pins
    from xrobot.generate_main import generate, validate_all, load_modules
    from xrobot.type_index import TypeIndex
    from xrobot.config_edit import sync_config
    project = _project(args)
    update = None
    if args.update is not None:
        update = list(args.update)
    lock = sync_modules(project, update, args.frozen, args.offline, args.context_ref, args.release_ref)
    print('Resolved %s' % _count(len(lock['modules']), 'Module commit'))
    if args.release_ref:
        check_tool_pins(project, args.release_ref, args.offline)
    modules = load_modules(project)
    index = TypeIndex.for_modules(modules)
    if update is not None:
        for config_path in project.configs():
            diff = sync_config(config_path, modules, index, project.relative(config_path))
            if diff:
                sys.stdout.write(diff)
    count = validate_all(project, modules, index)
    selected = project.selected_config()
    generate(project, selected)
    print('Checked %s; generated User/xrobot_main.hpp for %s'
          % (_count(count, 'config'), project.relative(selected)))
    _pin_warning(project)


def cmd_gen(args):
    from xrobot.generate_main import generate
    project = _project(args)
    config = _config_path(project, args.config)
    generate(project, config)
    print('Generated User/xrobot_main.hpp for %s' % project.relative(config or project.selected_config()))


def cmd_describe(args):
    from xrobot.describe import describe
    project = _project(args)
    result = describe(project, _config_path(project, args.config))
    out = sys.stdout
    if hasattr(out, 'reconfigure'):
        out.reconfigure(encoding='utf-8')
    json.dump(result, out, indent=2, ensure_ascii=False, default=str)
    out.write('\n')


def cmd_sync(args):
    from xrobot.config_edit import sync_config
    from xrobot.generate_main import load_modules
    from xrobot.type_index import TypeIndex
    project = _project(args)
    modules = load_modules(project)
    index = TypeIndex.for_modules(modules)
    paths = [_config_path(project, c) for c in args.config] if args.config else project.configs()
    for path in paths:
        diff = sync_config(path, modules, index, project.relative(path))
        if diff:
            sys.stdout.write(diff)


def cmd_format(args):
    from xrobot.config_edit import format_files
    project = _project(args)
    paths = [_config_path(project, c) for c in args.config] if args.config else project.configs()
    changed = format_files(paths, check=args.check)
    for path in changed:
        print(('needs formatting: %s' if args.check else 'formatted: %s') % project.relative(path))
    if args.check and changed:
        raise ConfigError('%d file(s) are not in the canonical layout; run `xrobot format`' % len(changed))


def cmd_instance(args):
    from xrobot import config_edit
    project = _project(args)
    config = _config_path(project, args.config) or project.selected_config()
    source = project.relative(config)
    if args.action == 'add':
        from xrobot.generate_main import load_modules
        from xrobot.type_index import TypeIndex
        modules = load_modules(project)
        identity = config_edit.add_instance(config, args.module, modules, TypeIndex.for_modules(modules),
                                           args.id, source)
        print('Added %s to %s; fill the null values (dependencies) before generating' % (identity, source))
    elif args.action == 'set':
        config_edit.set_value(config, args.id, args.path, config_edit.parse_json_value(args.value),
                             args.if_match, source)
    elif args.action == 'remove':
        config_edit.remove_instance(config, args.id, source)
    elif args.action == 'rename':
        config_edit.rename_instance(config, args.id, args.new_id, source)


def cmd_module(args):
    project = _project(args) if args.action != 'show' else None
    if args.action == 'add':
        from xrobot.config_edit import add_module
        add_module(project.modules_yaml, args.request)
        print('Added %s; run `xrobot setup` to fetch it' % args.request)
    elif args.action == 'remove':
        from xrobot.config_edit import remove_module
        remove_module(project.modules_yaml, args.request)
        print('Removed %s; run `xrobot setup` to update xrobot.lock' % args.request)
    else:
        from xrobot.module_parser import load_single_module, print_manifest
        print_manifest(load_single_module(Path(args.path)))


def cmd_new_module(args):
    from xrobot.module_creator import create_module
    path = create_module(args.name, args.desc, args.constructor, args.template, args.depends,
                         Path(args.out), args.include)
    print('Created %s' % path)


def cmd_check_module(args):
    from xrobot.generate_main import generate_compile_check, load_modules
    from xrobot.init_module import sync_modules
    project = _project(args)
    sync_modules(project, offline=args.offline)
    generate_compile_check(args.module, load_modules(project), args.output, args.template_arg)
    print('Generated %s' % args.output)


def cmd_source(args):
    from xrobot import source_manager
    sys.argv = ['xrobot source'] + args.rest
    source_manager.main()


def parser():
    top = argparse.ArgumentParser(prog='xrobot', description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    top.add_argument('--version', action='version', version='xrobot ' + __version__)
    top.add_argument('-C', dest='directory', default='.', help='start the BSP search at DIR')
    verbs = top.add_subparsers(dest='verb', required=True, metavar='<command>')

    verbs.add_parser('init', help='create Modules/modules.yaml, sources.yaml and User/xrobot.yaml here') \
        .set_defaults(run=cmd_init)

    setup = verbs.add_parser('setup', help='resolve Modules, check every config, regenerate the entry')
    group = setup.add_mutually_exclusive_group()
    group.add_argument('--update', nargs='*', metavar='MODULE',
                       help='re-resolve the named Modules (all when none is named)')
    group.add_argument('--frozen', action='store_true', help='restore exactly xrobot.lock')
    setup.add_argument('--offline', action='store_true')
    setup.add_argument('--context-ref', help='logical BSP branch/tag (refs/heads/... or refs/tags/...)')
    setup.add_argument('--release-ref', help='refuse unreleased commits for this target ref')
    setup.set_defaults(run=cmd_setup)

    gen = verbs.add_parser('gen', help='generate User/xrobot_main.hpp for one config (select a product)')
    gen.add_argument('-c', '--config', help='default: the selected product, else User/xrobot.yaml')
    gen.set_defaults(run=cmd_gen)

    desc = verbs.add_parser('describe', help='print the BSP state as JSON for editors')
    desc.add_argument('-c', '--config')
    desc.set_defaults(run=cmd_describe)

    sync = verbs.add_parser('sync', help='add new fields/parameters with defaults, drop removed ones')
    sync.add_argument('-c', '--config', action='append', default=[])
    sync.set_defaults(run=cmd_sync)

    fmt = verbs.add_parser('format', help='rewrite configs in the canonical layout')
    fmt.add_argument('-c', '--config', action='append', default=[])
    fmt.add_argument('--check', action='store_true', help='only report files that need formatting')
    fmt.set_defaults(run=cmd_format)

    instance = verbs.add_parser('instance', help='add, set, remove or rename one Module instance')
    instance.add_argument('-c', '--config')
    actions = instance.add_subparsers(dest='action', required=True)
    add = actions.add_parser('add')
    add.add_argument('module')
    add.add_argument('--id')
    change = actions.add_parser('set', help='replace one value: PATH is id, template_args[n] or '
                                            'args.<param>[.<field>|[n]]...; VALUE is JSON')
    change.add_argument('id')
    change.add_argument('path')
    change.add_argument('value')
    change.add_argument('--if-match', help='sha256 of the LF-normalized file the edit was based on')
    remove = actions.add_parser('remove')
    remove.add_argument('id')
    rename = actions.add_parser('rename')
    rename.add_argument('id')
    rename.add_argument('new_id')
    instance.set_defaults(run=cmd_instance)

    module = verbs.add_parser('module', help='add/remove a Module request, or show a Module interface')
    module_actions = module.add_subparsers(dest='action', required=True)
    module_actions.add_parser('add').add_argument('request', help='owner/Repo[@ref]')
    module_actions.add_parser('remove').add_argument('request', help='owner/Repo')
    module_actions.add_parser('show').add_argument('path', help='Module folder or header')
    module.set_defaults(run=cmd_module)

    new = verbs.add_parser('new-module', help='create a Module skeleton')
    new.add_argument('name')
    new.add_argument('--desc', default='')
    new.add_argument('--constructor', action='append', default=[],
                     help='a C++ parameter declaration; repeat for each parameter')
    new.add_argument('--template', action='append', default=[])
    new.add_argument('--include', action='append', default=[])
    new.add_argument('--depends', nargs='*', default=[])
    new.add_argument('--out', default='.')
    new.set_defaults(run=cmd_new_module)

    check = verbs.add_parser('check-module', help='write a never-executed constructor call for CI')
    check.add_argument('module')
    check.add_argument('-o', '--output', default='module_check.cpp')
    check.add_argument('--template-arg', action='append', default=[])
    check.add_argument('--offline', action='store_true')
    check.set_defaults(run=cmd_check_module)

    source = verbs.add_parser('source', help='query or edit module catalogs (sources.yaml, index.yaml)')
    source.add_argument('rest', nargs=argparse.REMAINDER)
    source.set_defaults(run=cmd_source)
    return top


def main(argv=None):
    top = parser()
    args, extra = top.parse_known_args(argv)
    if extra and args.verb != 'source':
        top.error('unrecognized arguments: %s' % ' '.join(extra))
    if args.verb == 'source':
        args.rest = extra + list(args.rest)
    try:
        args.run(args)
    except (ConfigError, ProjectError, OSError, ValueError, yaml.YAMLError) as error:
        sys.stderr.write('%s\n' % error)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
