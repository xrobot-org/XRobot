"""xrobot 命令行：解析模块、生成静态入口、编辑配置。
The xrobot command line: resolve Modules, generate the static entry and edit configurations.
"""

import argparse
import json
import re
import sys
from collections.abc import Sequence
from pathlib import Path

import yaml

from xrobot import __version__
from xrobot.config import ConfigError, parse_yaml
from xrobot.project import Project, ProjectError

DESCRIPTION = """\
Resolve Modules, generate the static entry and edit configurations.

Commands work on the BSP that contains the current directory: the nearest
directory at or above it with Modules/modules.yaml. -C DIR starts the search
at DIR instead."""

INIT_MODULES = f"xrobot: {__version__}\nmodules: []\n"
INIT_CONFIG = "modules: []\nsettings:\n  monitor_sleep_ms: 1000\n"
IGNORED = ("/User/xrobot_main.hpp", "/Modules/CMakeLists.txt", "/Modules/*/")
COMMIT = re.compile(r"[0-9a-f]{40}")


def _project(args: argparse.Namespace) -> Project:
    """从 -C 目录（默认当前目录）向上找到的 BSP。
    The BSP found at or above the -C directory (default: the current directory).
    """
    return Project.discover(args.directory)


def _config_path(project: Project, value: str | None) -> Path | None:
    """解析 -c 给出的配置路径：先相对当前目录，不存在时相对 BSP 根目录。
    Resolve a -c configuration path: relative to the current directory when that exists,
    else relative to the BSP root.
    """
    if value is None:
        return None
    path = Path(value)
    if not path.is_absolute():
        candidate = Path.cwd() / path
        path = candidate if candidate.exists() else project.root / path
    return path.resolve()


def _count(number: int, noun: str) -> str:
    """数量加名词，按数量选择单复数。
    A count followed by the noun in singular or plural.
    """
    return f"{number} {noun}{'' if number == 1 else 's'}"


def _check_pin(project: Project, frozen: bool = False) -> None:
    """比较安装的 XRobot 与 Modules/modules.yaml 中固定的版本。
    Compare the installed XRobot with the version pinned in Modules/modules.yaml.

    不一致时给出警告，--frozen 下报错。固定为提交号时无法与安装的版本比较，不作检查。
    A difference is a warning, and an error with --frozen. A commit pin cannot be compared
    with the installed version and is not checked.

    Raises:
        ProjectError: --frozen 下未固定版本或版本不一致。
            With --frozen, XRobot is not pinned or the versions differ.
    """
    from xrobot.lock import read_modules_yaml

    _, pin = read_modules_yaml(project.modules_yaml)
    if pin is None:
        problem = f"Modules/modules.yaml does not pin XRobot; add `xrobot: {__version__}`"
    elif pin != __version__ and not COMMIT.fullmatch(pin):
        problem = f"installed XRobot {__version__} differs from the pinned {pin}"
    else:
        return
    if frozen:
        raise ProjectError(f"{problem} (--frozen requires the pinned version)")
    print(f"warning: {problem}", file=sys.stderr)


def _add_ignore_entries(path: Path) -> bool:
    """把生成的文件追加到 .gitignore，已有内容和换行符保持不变。
    Append the generated files to .gitignore, keeping the existing content and line endings.

    Returns:
        是否追加了条目。
        Whether any entry was appended.
    """
    from xrobot.project import atomic_write

    text = path.read_bytes().decode("utf-8") if path.exists() else ""
    missing = [entry for entry in IGNORED if entry not in text.splitlines()]
    if not missing:
        return False
    newline = "\r\n" if "\r\n" in text else "\n"
    if text and not text.endswith("\n"):
        text += newline
    atomic_write(path, text + newline.join(missing) + newline)
    return True


def cmd_init(args: argparse.Namespace) -> None:
    """在 -C 目录（默认当前目录）创建 BSP 的 XRobot 文件，已有的文件保持不变。
    Create the XRobot files of a BSP in the -C directory (default: the current directory);
    existing files are kept.
    """
    from xrobot.project import atomic_write
    from xrobot.source_manager import SOURCES_TEMPLATE

    root = Path(args.directory).resolve()
    created = []
    for relative, text in (
        ("Modules/modules.yaml", INIT_MODULES),
        ("Modules/sources.yaml", SOURCES_TEMPLATE),
        ("User/xrobot.yaml", INIT_CONFIG),
    ):
        path = root / relative
        if not path.exists():
            atomic_write(path, text)
            created.append(relative)
    if _add_ignore_entries(root / ".gitignore"):
        created.append(".gitignore entries")
    print(f"Created {', '.join(created) if created else 'nothing (already initialized)'}")


def cmd_setup(args: argparse.Namespace) -> None:
    """解析并检出模块，检查所有配置，为选中的配置重新生成入口。
    Resolve and check out the Modules, check every configuration and regenerate the entry
    for the selected one.
    """
    from xrobot.config_edit import sync_config
    from xrobot.generate_main import generate, load_modules, validate_all
    from xrobot.lock import check_tool_pins, sync_modules
    from xrobot.type_index import TypeIndex

    project = _project(args)
    _check_pin(project, args.frozen)
    update = list(args.update) if args.update is not None else None
    lock = sync_modules(
        project, update, args.frozen, args.offline, args.context_ref, args.release_ref
    )
    print(f"Resolved {_count(len(lock['modules']), 'Module commit')}")
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
    print(
        f"Checked {_count(count, 'config')}; generated User/xrobot_main.hpp for "
        f"{project.relative(selected)}"
    )


def cmd_gen(args: argparse.Namespace) -> None:
    """为一份配置生成 User/xrobot_main.hpp，并将其记为选中的配置。
    Generate User/xrobot_main.hpp for one configuration, which becomes the selected one.
    """
    from xrobot.generate_main import generate

    project = _project(args)
    _check_pin(project)
    config = _config_path(project, args.config)
    generate(project, config)
    print(
        f"Generated User/xrobot_main.hpp for {project.relative(config or project.selected_config())}"
    )


def cmd_describe(args: argparse.Namespace) -> None:
    """以 JSON 输出 BSP 的状态，供编辑器使用。
    Print the state of the BSP as JSON for editors.
    """
    from xrobot.describe import describe

    project = _project(args)
    result = describe(project, _config_path(project, args.config))
    json.dump(result, sys.stdout, indent=2, ensure_ascii=False, default=str)
    sys.stdout.write("\n")


def cmd_sync(args: argparse.Namespace) -> None:
    """按模块当前的接口更新配置，并输出改动。
    Update configurations to the current Module interfaces and print the changes.
    """
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


def cmd_format(args: argparse.Namespace) -> None:
    """按规范格式重写配置，或用 --check 只报告需要重写的文件。
    Rewrite configurations in the canonical layout, or with --check only report the files
    that need it.

    Raises:
        ConfigError: --check 发现需要重写的文件。
            --check found files that need formatting.
    """
    from xrobot.config_edit import format_files

    project = _project(args)
    paths = [_config_path(project, c) for c in args.config] if args.config else project.configs()
    changed = format_files(paths, check=args.check)
    label = "needs formatting" if args.check else "formatted"
    for path in changed:
        print(f"{label}: {project.relative(path)}")
    if args.check and changed:
        raise ConfigError(
            f"Found {_count(len(changed), 'file')} not in the canonical layout; run `xrobot format`"
        )


def parse_value(text: str, as_json: bool = False) -> object:
    """解析命令行给出的值。默认按配置文件的规则读成一个 YAML 值：不加引号或用单引号的是 C++ 代码，
    双引号的是 C++ 字符串，空值表示未填写；as_json 时按 JSON 读，JSON 字符串是 C++ 文本。
    Parse a value given on the command line. By default it is one YAML value read like a
    configuration value: C++ code without quotes or in single quotes, a C++ string in double
    quotes, not filled in when empty. With as_json it is JSON whose strings are C++ text.

    Raises:
        ConfigError: 不是有效的 YAML 值，或 as_json 时不是 JSON。
            The text is not a valid YAML value, or not JSON with as_json.
    """
    if as_json:
        try:
            return json.loads(text)
        except json.JSONDecodeError as error:
            raise ConfigError(f"VALUE is not JSON: {error}") from error
    if not text.strip():
        return None
    return parse_yaml(text, "VALUE")


def cmd_instance(args: argparse.Namespace) -> None:
    """在一份配置中添加、修改、删除或重命名模块实例。
    Add, set, remove or rename a Module instance in one configuration.
    """
    from xrobot import config_edit

    project = _project(args)
    config = _config_path(project, args.config) or project.selected_config()
    source = project.relative(config)
    if args.action == "add":
        from xrobot.generate_main import load_modules
        from xrobot.type_index import TypeIndex

        modules = load_modules(project)
        identity = config_edit.add_instance(
            config, args.module, modules, TypeIndex.for_modules(modules), args.id, source
        )
        print(
            f"Added {identity} to {source}; fill the null values (dependencies) before generating"
        )
    elif args.action == "set":
        config_edit.set_value(
            config, args.id, args.path, parse_value(args.value, args.json), args.if_match, source
        )
    elif args.action == "remove":
        config_edit.remove_instance(config, args.id, source)
    elif args.action == "rename":
        config_edit.rename_instance(config, args.id, args.new_id, source)


def _module_target(args: argparse.Namespace) -> Path:
    """module show 读取的目标：已存在的路径，否则为 BSP 中锁定的同名模块。
    What `module show` reads: an existing path, else the locked Module of the BSP with that id.

    Raises:
        ProjectError: 路径不存在，也不在 BSP 中。
            The path does not exist and there is no BSP to look the id up in.
        ValueError: BSP 中没有这个模块，或名称有歧义。
            The BSP has no such Module, or the name is ambiguous.
    """
    path = Path(args.module)
    if path.exists():
        return path
    from xrobot.generate_main import load_modules
    from xrobot.module_parser import select_module

    try:
        project = _project(args)
    except ProjectError as error:
        raise ProjectError(f"{args.module}: not a file or folder; {error}") from error
    try:
        return select_module(load_modules(project), args.module)["path"]
    except ValueError as error:
        raise ValueError(f"{args.module}: not a file or folder; {error}") from error


def cmd_module(args: argparse.Namespace) -> None:
    """在 modules.yaml 中添加或删除模块请求，或输出一个模块的清单和构造函数。
    Add or remove a Module request in modules.yaml, or print a Module's manifest and
    constructors.
    """
    if args.action == "show":
        from xrobot.module_parser import load_single_module, print_manifest

        print_manifest(load_single_module(_module_target(args)))
        return
    project = _project(args)
    if args.action == "add":
        from xrobot.config_edit import add_module

        add_module(project.modules_yaml, args.request)
        print(f"Added {args.request}; run `xrobot setup` to fetch it")
    else:
        from xrobot.config_edit import remove_module

        remove_module(project.modules_yaml, args.request)
        print(f"Removed {args.request}; run `xrobot setup` to update xrobot.lock")


def cmd_new_module(args: argparse.Namespace) -> None:
    """创建模块骨架。
    Create a Module skeleton.
    """
    from xrobot.module_creator import create_module

    path = create_module(
        args.name,
        args.desc,
        args.constructor,
        args.template,
        args.depends,
        Path(args.out),
        args.include,
    )
    print(f"Created {path}")


def cmd_check_module(args: argparse.Namespace) -> None:
    """像 setup 一样解析模块，然后写出模块 CI 编译用的构造调用。
    Resolve the Modules as setup does, then write the constructor call that Module CI compiles.
    """
    from xrobot.generate_main import generate_compile_check, load_modules
    from xrobot.lock import sync_modules

    project = _project(args)
    sync_modules(project, offline=args.offline)
    generate_compile_check(args.module, load_modules(project), args.output, args.template_arg)
    print(f"Generated {args.output}")


def cmd_source(args: argparse.Namespace) -> None:
    """查询或编辑源（sources.yaml 与 index.yaml）。
    Query or edit Sources (sources.yaml and index.yaml).

    Raises:
        ProjectError: 未指定 --sources 且不在 BSP 中，或要查询的 sources.yaml 不存在。
            No --sources outside a BSP, or the sources.yaml to query does not exist.
    """
    from xrobot import source_manager

    if args.action == "create-index":
        source_manager.create_index_yaml(args.output, args.namespace, args.mirror_of)
        return
    if args.action == "add-index":
        source_manager.add_index_entry(args.index, args.repo_url)
        return
    sources = Path(args.sources) if args.sources else _project(args).sources_yaml
    if args.action == "create-sources":
        source_manager.create_sources_yaml(Path(args.output) if args.output else sources)
        return
    if args.action == "add-source":
        source_manager.add_source(sources, args.url, args.priority)
        return
    if not sources.is_file():
        raise ProjectError(f"{sources} does not exist; run `xrobot source create-sources`")
    manager = source_manager.SourceManager(sources)
    if args.action in ("get", "find"):
        identity = manager.resolve_id(args.id)
        if args.action == "get":
            value = manager.packages[identity]
        else:
            value = [{"repo": r, "source": s.url} for r, s in manager.find_module(identity)]
        sys.stdout.write(yaml.safe_dump(value, sort_keys=False, allow_unicode=True))
        return
    for identity, record in sorted(manager.packages.items()):
        if args.type and args.type != record["type"]:
            continue
        if (
            args.action == "search"
            and args.query.casefold()
            not in json.dumps(record, ensure_ascii=False, default=str).casefold()
        ):
            continue
        print(f"{identity} [{record['type']}] {record['repo']}")


def _source_parser(verbs: argparse._SubParsersAction) -> None:
    """添加 source 命令及其子命令。
    Add the source command and its actions.
    """
    source = verbs.add_parser("source", help="query or edit Sources (sources.yaml, index.yaml)")
    source.add_argument(
        "--sources", metavar="FILE", help="sources.yaml to use (default: the BSP's)"
    )
    actions = source.add_subparsers(dest="action", required=True, metavar="<action>")
    for name, text in (
        ("list", "list the packages of all Sources"),
        ("search", "list the packages whose entry contains TEXT"),
    ):
        action = actions.add_parser(name, help=text)
        if name == "search":
            action.add_argument("query", metavar="TEXT")
        action.add_argument("--type", choices=["module", "bsp"], help="only this kind of package")
    actions.add_parser("get", help="print the entry of a package").add_argument(
        "id", help="owner/Repo, or Repo when unique"
    )
    actions.add_parser(
        "find", help="list every Source (mirrors included) that provides a package"
    ).add_argument("id", help="owner/Repo, or Repo when unique")
    create = actions.add_parser(
        "create-sources", help="write a sources.yaml with the official Source"
    )
    create.add_argument(
        "-o", "--output", metavar="FILE", help="default: the sources.yaml of --sources or the BSP"
    )
    add = actions.add_parser("add-source", help="add a Source to sources.yaml")
    add.add_argument("url", help="URL of an index.yaml, or a path relative to sources.yaml")
    add.add_argument(
        "--priority",
        type=int,
        default=0,
        help="the lower value wins when Sources list the same package (default: 0)",
    )
    index = actions.add_parser("create-index", help="write a new index.yaml")
    index.add_argument(
        "-o", "--output", metavar="FILE", default="Modules/index.yaml", help="default: %(default)s"
    )
    index.add_argument(
        "--namespace",
        default="local",
        help="owner of repositories outside GitHub (default: %(default)s)",
    )
    index.add_argument("--mirror-of", metavar="NAMESPACE", help="mark the index as a mirror")
    entry = actions.add_parser("add-index", help="add a Module repository to an index.yaml")
    entry.add_argument("repo_url", help="Git URL of the Module repository")
    entry.add_argument("--index", metavar="FILE", required=True, help="the index.yaml to edit")
    source.set_defaults(run=cmd_source)


def parser() -> argparse.ArgumentParser:
    """构造 xrobot 的参数解析器。
    Build the xrobot argument parser.
    """
    top = argparse.ArgumentParser(
        prog="xrobot", description=DESCRIPTION, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    top.add_argument("--version", action="version", version=f"xrobot {__version__}")
    top.add_argument(
        "-C",
        dest="directory",
        metavar="DIR",
        default=".",
        help="start the BSP search at DIR (init: create the files in DIR)",
    )
    verbs = top.add_subparsers(dest="verb", required=True, metavar="<command>")

    verbs.add_parser(
        "init",
        help="create Modules/modules.yaml, Modules/sources.yaml and User/xrobot.yaml",
    ).set_defaults(run=cmd_init)

    setup = verbs.add_parser(
        "setup", help="resolve Modules, check every config, regenerate the entry"
    )
    group = setup.add_mutually_exclusive_group()
    group.add_argument(
        "--update",
        nargs="*",
        metavar="MODULE",
        help="re-resolve the named Modules (all when none is named) and sync every config",
    )
    group.add_argument(
        "--frozen",
        action="store_true",
        help="check out exactly xrobot.lock; the installed XRobot must match the pin",
    )
    setup.add_argument(
        "--offline",
        action="store_true",
        help="use the Modules already in Modules/ without fetching (needs xrobot.lock)",
    )
    setup.add_argument(
        "--context-ref",
        metavar="REF",
        help="logical BSP branch or tag (refs/heads/... or refs/tags/...)",
    )
    setup.add_argument(
        "--release-ref", metavar="REF", help="refuse unreleased commits for this target ref"
    )
    setup.set_defaults(run=cmd_setup)

    gen = verbs.add_parser(
        "gen", help="generate User/xrobot_main.hpp for one config and select that config"
    )
    gen.add_argument("-c", "--config", help="default: the selected config, else User/xrobot.yaml")
    gen.set_defaults(run=cmd_gen)

    desc = verbs.add_parser("describe", help="print the BSP state as JSON for editors")
    desc.add_argument("-c", "--config", help="default: the selected config")
    desc.set_defaults(run=cmd_describe)

    for name, run, text in (
        ("sync", cmd_sync, "add new fields and parameters with defaults, drop removed ones"),
        ("format", cmd_format, "rewrite configs in the canonical layout"),
    ):
        command = verbs.add_parser(name, help=text)
        command.add_argument(
            "-c",
            "--config",
            action="append",
            default=[],
            help="config to process; repeat for several (default: all)",
        )
        if name == "format":
            command.add_argument(
                "--check", action="store_true", help="only report files that need formatting"
            )
        command.set_defaults(run=run)

    instance = verbs.add_parser("instance", help="add, set, remove or rename one Module instance")
    instance.add_argument("-c", "--config", help="config to edit (default: the selected config)")
    actions = instance.add_subparsers(dest="action", required=True, metavar="<action>")
    add = actions.add_parser("add", help="add an instance with the default values")
    add.add_argument("module", help="owner/Repo, or Repo when unique")
    add.add_argument("--id", help="instance id (default: <module>_<n>)")
    change = actions.add_parser(
        "set",
        help="replace one value",
        description="PATH is template_args[n] or args.<param>[.<field>|[n]]...; change an id\n"
        "with `xrobot instance rename`.\n"
        "VALUE is one YAML value, read like a value in the config: C++ code without quotes\n"
        "or in single quotes, a C++ string in double quotes (e.g. '\"bmi088_gyro\"').",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    change.add_argument("id", metavar="ID", help="instance id")
    change.add_argument("path", metavar="PATH", help="value path, e.g. args.param.cycle")
    change.add_argument("value", metavar="VALUE", help="new value")
    change.add_argument(
        "--json", action="store_true", help="VALUE is JSON whose strings are C++ text"
    )
    change.add_argument(
        "--if-match",
        metavar="SHA256",
        help="sha256 of the LF-normalized file the edit was based on",
    )
    actions.add_parser("remove", help="remove an instance").add_argument("id", help="instance id")
    rename = actions.add_parser(
        "rename", help="rename an instance and the references to it in the config"
    )
    rename.add_argument("id", help="instance id")
    rename.add_argument("new_id", help="new instance id")
    instance.set_defaults(run=cmd_instance)

    module = verbs.add_parser(
        "module", help="add/remove a Module request, or show a Module interface"
    )
    module_actions = module.add_subparsers(dest="action", required=True, metavar="<action>")
    module_actions.add_parser("add", help="request a Module in modules.yaml").add_argument(
        "request", help="owner/Repo[@ref]"
    )
    module_actions.add_parser("remove", help="remove a Module request").add_argument(
        "request", help="owner/Repo"
    )
    module_actions.add_parser(
        "show", help="print the manifest and constructors of a Module"
    ).add_argument(
        "module", help="Module folder or header, or the id of a locked Module (owner/Repo or Repo)"
    )
    module.set_defaults(run=cmd_module)

    new = verbs.add_parser("new-module", help="create a Module skeleton")
    new.add_argument("name", help="class and folder name (a C++ identifier)")
    new.add_argument("--desc", metavar="TEXT", default="", help="one-line description")
    new.add_argument(
        "--constructor",
        metavar="DECL",
        action="append",
        default=[],
        help="a C++ constructor parameter declaration; repeat for each parameter",
    )
    new.add_argument(
        "--template",
        metavar="DECL",
        action="append",
        default=[],
        help="a C++ template parameter declaration; repeat for each parameter",
    )
    new.add_argument(
        "--include",
        metavar="HEADER",
        action="append",
        default=[],
        help="a header to #include; repeat for each header",
    )
    new.add_argument(
        "--depends",
        metavar="MODULE",
        nargs="*",
        default=[],
        help="Modules this one depends on (owner/Repo[@ref])",
    )
    new.add_argument("--out", metavar="DIR", default=".", help="parent folder (default: .)")
    new.set_defaults(run=cmd_new_module)

    check = verbs.add_parser(
        "check-module",
        help="resolve Modules as setup does (updates xrobot.lock and Modules/), then write "
        "a never-executed constructor call for CI",
    )
    check.add_argument("module", help="owner/Repo, or Repo when unique")
    check.add_argument(
        "-o", "--output", metavar="FILE", default="module_check.cpp", help="default: %(default)s"
    )
    check.add_argument(
        "--template-arg",
        metavar="ARG",
        action="append",
        default=[],
        help="a template argument; repeat for each argument",
    )
    check.add_argument("--offline", action="store_true", help="use the Modules already in Modules/")
    check.set_defaults(run=cmd_check_module)

    _source_parser(verbs)
    return top


def _utf8_output() -> None:
    """让 stdout 和 stderr 以 UTF-8 输出，与平台默认编码无关（重定向时也是）。
    Write stdout and stderr as UTF-8 whatever the platform encoding, also when redirected.
    """
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")


def main(argv: Sequence[str] | None = None) -> int:
    """运行 xrobot 命令；出错时输出一行信息并返回 1。
    Run an xrobot command; on an error print one line and return 1.
    """
    _utf8_output()
    args = parser().parse_args(argv)
    try:
        args.run(args)
    except (ConfigError, ProjectError, OSError, ValueError, yaml.YAMLError) as error:
        sys.stderr.write(f"{error}\n")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
