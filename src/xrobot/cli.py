"""xrobot 命令行：解析模块、生成静态入口、编辑配置。
The xrobot command line: resolve Modules, generate the static entry and edit configurations.
"""

import argparse
import gc
import json
import re
import sys
from collections.abc import Sequence
from pathlib import Path

import yaml
from xr_syntax.i18n import chinese, localize_argparse, tr

from xrobot import __version__
from xrobot.config import ConfigError, parse_yaml
from xrobot.project import Project, ProjectError


def _description() -> str:
    """xrobot --help 开头的说明。
    The description that opens xrobot --help.
    """
    return tr(
        "Resolve Modules, generate the static entry and edit configurations.\n\n"
        "Commands work on the BSP that contains the current directory: the nearest\n"
        "directory at or above it with Modules/modules.yaml. -C DIR starts the search\n"
        "at DIR instead.",
        "解析模块、生成静态入口、编辑配置。\n\n"
        "命令作用于包含当前目录的 BSP，即当前目录或其上层中最近的含有\n"
        "Modules/modules.yaml 的目录。-C DIR 改为从 DIR 开始查找。",
    )


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


def _count(number: int, noun: str, chinese_noun: str) -> str:
    """数量加名词：英文按数量选择单复数，中文用量词“个”。
    A count followed by the noun: singular or plural in English, with the measure word 个
    in Chinese.
    """
    return tr(f"{number} {noun}{'' if number == 1 else 's'}", f"{number} 个{chinese_noun}")


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
        problem = tr(
            f"Modules/modules.yaml does not pin XRobot; add `xrobot: {__version__}`",
            f"Modules/modules.yaml 没有固定 XRobot 的版本；请添加 `xrobot: {__version__}`",
        )
    elif pin != __version__ and not COMMIT.fullmatch(pin):
        problem = tr(
            f"installed XRobot {__version__} differs from the pinned {pin}",
            f"安装的 XRobot {__version__} 与固定的版本 {pin} 不同",
        )
    else:
        return
    if frozen:
        raise ProjectError(
            tr(
                f"{problem} (--frozen requires the pinned version)",
                f"{problem}（--frozen 要求使用固定的版本）",
            )
        )
    print(tr("warning: ", "警告：") + problem, file=sys.stderr)


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
        created.append(tr(".gitignore entries", ".gitignore 条目"))
    if created:
        print(tr(f"Created {', '.join(created)}", f"已创建 {'、'.join(created)}"))
    else:
        print(tr("Created nothing (already initialized)", "没有需要创建的文件（已经初始化）"))


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
    count = _count(len(lock["modules"]), "Module commit", "模块提交")
    print(tr(f"Resolved {count}", f"已解析 {count}"))
    if args.release_ref:
        check_tool_pins(project, args.release_ref, args.offline)
    modules = load_modules(project)
    index = TypeIndex.for_modules(modules)
    if update is not None:
        for config_path in project.configs():
            diff = sync_config(config_path, modules, index, project.relative(config_path))
            if diff:
                sys.stdout.write(diff)
    checked = _count(validate_all(project, modules, index), "config", "配置")
    selected = project.selected_config()
    generate(project, selected, modules, index)
    name = project.relative(selected)
    print(
        tr(
            f"Checked {checked}; generated User/xrobot_main.hpp for {name}",
            f"已检查 {checked}；已为 {name} 生成 User/xrobot_main.hpp",
        )
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
    name = project.relative(config or project.selected_config())
    print(
        tr(f"Generated User/xrobot_main.hpp for {name}", f"已为 {name} 生成 User/xrobot_main.hpp")
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
    label = tr("needs formatting", "需要格式化") if args.check else tr("formatted", "已格式化")
    for path in changed:
        print(f"{label}: {project.relative(path)}")
    if args.check and changed:
        files = _count(len(changed), "file", "文件")
        raise ConfigError(
            tr(
                f"Found {files} not in the canonical layout; run `xrobot format`",
                f"有 {files}不符合规范格式；请运行 `xrobot format`",
            )
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
            raise ConfigError(
                tr(f"VALUE is not JSON: {error}", f"VALUE 不是 JSON：{error}")
            ) from error
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
            tr(
                f"Added {identity} to {source}; fill the null values (dependencies) before "
                "generating",
                f"已将 {identity} 添加到 {source}；生成前请填写值为空的依赖参数",
            )
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
        raise ProjectError(
            tr(
                f"{args.module}: not a file or folder; {error}",
                f"{args.module}: 不是文件或文件夹；{error}",
            )
        ) from error
    try:
        return select_module(load_modules(project), args.module)["path"]
    except ValueError as error:
        raise ValueError(
            tr(
                f"{args.module}: not a file or folder; {error}",
                f"{args.module}: 不是文件或文件夹；{error}",
            )
        ) from error


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
        print(
            tr(
                f"Added {args.request}; run `xrobot setup` to fetch it",
                f"已添加 {args.request}；请运行 `xrobot setup` 获取它",
            )
        )
    else:
        from xrobot.config_edit import remove_module

        remove_module(project.modules_yaml, args.request)
        print(
            tr(
                f"Removed {args.request}; run `xrobot setup` to update xrobot.lock",
                f"已删除 {args.request}；请运行 `xrobot setup` 更新 xrobot.lock",
            )
        )


def cmd_new_module(args: argparse.Namespace) -> None:
    """创建模块骨架。
    Create a Module skeleton.
    """
    from xrobot.module_creator import create_module

    path = create_module(
        args.name,
        description=args.desc,
        constructor_args=args.constructor,
        template_args=args.template,
        depends=args.depends,
        output_dir=Path(args.out),
        includes=args.include,
        ci_template_args=args.template_arg,
    )
    print(tr(f"Created {path}", f"已创建 {path}"))


def cmd_check_module(args: argparse.Namespace) -> None:
    """像 setup 一样解析模块，然后写出模块 CI 编译用的构造调用。
    Resolve the Modules as setup does, then write the constructor call that Module CI compiles.
    """
    from xrobot.generate_main import generate_compile_check, load_modules
    from xrobot.lock import sync_modules

    project = _project(args)
    sync_modules(project, offline=args.offline)
    generate_compile_check(args.module, load_modules(project), args.output, args.template_arg)
    print(tr(f"Generated {args.output}", f"已生成 {args.output}"))


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
        raise ProjectError(
            tr(
                f"{sources} does not exist; run `xrobot source create-sources`",
                f"{sources} 不存在；请运行 `xrobot source create-sources`",
            )
        )
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


def _command(
    actions: argparse._SubParsersAction, name: str, text: str, details: str | None = None
) -> argparse.ArgumentParser:
    """添加一个子命令；命令列表中的一句话说明也作为它自己 --help 的开头。
    Add a subcommand; its one-line help in the command list also opens its own --help.

    Args:
        actions: 所属命令的子命令集合。
            The subcommands of the parent command.
        name: 子命令名。
            The subcommand name.
        text: 一句话说明，小写开头、不带句号。
            The one-line help, lowercase and without a period.
        details: 显示在说明之后的更多内容，按原样换行。
            More text shown after the help, with its line breaks kept.
    """
    description = text + "。" if chinese() else text[0].upper() + text[1:] + "."
    if not details:
        return actions.add_parser(name, help=text, description=description)
    return actions.add_parser(
        name,
        help=text,
        description=description + "\n\n" + details,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )


def _source_parser(verbs: argparse._SubParsersAction) -> None:
    """添加 source 命令及其子命令。
    Add the source command and its actions.
    """
    source = _command(
        verbs,
        "source",
        tr(
            "query or edit Sources (sources.yaml, index.yaml)",
            "查询或编辑源（sources.yaml、index.yaml）",
        ),
    )
    source.add_argument(
        "--sources",
        metavar="FILE",
        help=tr(
            "sources.yaml to use (default: the BSP's)", "使用的 sources.yaml（默认：BSP 中的）"
        ),
    )
    actions = source.add_subparsers(dest="action", required=True, metavar="<action>")
    package_id = tr("owner/Repo, or Repo when unique", "owner/Repo；唯一时可只写 Repo")
    for name, text in (
        ("list", tr("list the packages of all Sources", "列出所有源中的包")),
        ("search", tr("list the packages whose entry contains TEXT", "列出条目中包含 TEXT 的包")),
    ):
        action = _command(actions, name, text)
        if name == "search":
            action.add_argument("query", metavar="TEXT")
        action.add_argument(
            "--type",
            choices=["module", "bsp"],
            help=tr("only this kind of package", "只列出这一类包"),
        )
    _command(actions, "get", tr("print the entry of a package", "输出一个包的条目")).add_argument(
        "id", help=package_id
    )
    _command(
        actions,
        "find",
        tr(
            "list every Source (mirrors included) that provides a package",
            "列出提供某个包的所有源（含镜像）",
        ),
    ).add_argument("id", help=package_id)
    create = _command(
        actions,
        "create-sources",
        tr("write a sources.yaml with the official Source", "写出一份包含官方源的 sources.yaml"),
    )
    create.add_argument(
        "-o",
        "--output",
        metavar="FILE",
        help=tr(
            "default: the sources.yaml of --sources or the BSP",
            "默认：--sources 指定的或 BSP 中的 sources.yaml",
        ),
    )
    add = _command(
        actions, "add-source", tr("add a Source to sources.yaml", "向 sources.yaml 添加一个源")
    )
    add.add_argument(
        "url",
        help=tr(
            "URL of an index.yaml, or a path relative to sources.yaml",
            "index.yaml 的 URL，或相对 sources.yaml 的路径",
        ),
    )
    add.add_argument(
        "--priority",
        type=int,
        default=0,
        help=tr(
            "the lower value wins when Sources list the same package (default: 0)",
            "多个源列出同一个包时，数值小的优先（默认：0）",
        ),
    )
    index = _command(
        actions, "create-index", tr("write a new index.yaml", "写出一份新的 index.yaml")
    )
    index.add_argument(
        "-o",
        "--output",
        metavar="FILE",
        default="Modules/index.yaml",
        help=tr("default: %(default)s", "默认：%(default)s"),
    )
    index.add_argument(
        "--namespace",
        default="local",
        help=tr(
            "owner of repositories outside GitHub (default: %(default)s)",
            "GitHub 以外仓库的所有者（默认：%(default)s）",
        ),
    )
    index.add_argument(
        "--mirror-of",
        metavar="NAMESPACE",
        help=tr("mark the index as a mirror", "把这份 index 标记为镜像"),
    )
    entry = _command(
        actions,
        "add-index",
        tr("add a Module repository to an index.yaml", "向 index.yaml 添加一个模块仓库"),
    )
    entry.add_argument(
        "repo_url", help=tr("Git URL of the Module repository", "模块仓库的 Git URL")
    )
    entry.add_argument(
        "--index",
        metavar="FILE",
        required=True,
        help=tr("the index.yaml to edit", "要编辑的 index.yaml"),
    )
    source.set_defaults(run=cmd_source)


def parser() -> argparse.ArgumentParser:
    """构造 xrobot 的参数解析器。
    Build the xrobot argument parser.
    """
    localize_argparse()
    top = argparse.ArgumentParser(
        prog="xrobot",
        description=_description(),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    top.add_argument("--version", action="version", version=f"xrobot {__version__}")
    top.add_argument(
        "-C",
        dest="directory",
        metavar="DIR",
        default=".",
        help=tr(
            "start the BSP search at DIR (init: create the files in DIR)",
            "从 DIR 开始查找 BSP（init：在 DIR 中创建文件）",
        ),
    )
    verbs = top.add_subparsers(dest="verb", required=True, metavar="<command>")

    package_id = tr("owner/Repo, or Repo when unique", "owner/Repo；唯一时可只写 Repo")
    instance_id = tr("instance id", "实例 id")
    _command(
        verbs,
        "init",
        tr(
            "create Modules/modules.yaml, Modules/sources.yaml and User/xrobot.yaml",
            "创建 Modules/modules.yaml、Modules/sources.yaml 和 User/xrobot.yaml",
        ),
    ).set_defaults(run=cmd_init)

    setup = _command(
        verbs,
        "setup",
        tr(
            "resolve Modules, check every config, regenerate the entry",
            "解析模块，检查所有配置，重新生成入口",
        ),
    )
    group = setup.add_mutually_exclusive_group()
    group.add_argument(
        "--update",
        nargs="*",
        metavar="MODULE",
        help=tr(
            "re-resolve the named Modules (all when none is named) and sync every config",
            "重新解析指定的模块（未指定时为全部模块），并同步所有配置",
        ),
    )
    group.add_argument(
        "--frozen",
        action="store_true",
        help=tr(
            "check out exactly xrobot.lock; the installed XRobot must match the pin",
            "严格按 xrobot.lock 检出；安装的 XRobot 必须与固定的版本一致",
        ),
    )
    setup.add_argument(
        "--offline",
        action="store_true",
        help=tr(
            "use the Modules already in Modules/ without fetching (needs xrobot.lock)",
            "使用 Modules/ 中已有的模块，不联网获取（需要 xrobot.lock）",
        ),
    )
    setup.add_argument(
        "--context-ref",
        metavar="REF",
        help=tr(
            "logical BSP branch or tag (refs/heads/... or refs/tags/...)",
            "BSP 逻辑上所在的分支或 tag（refs/heads/... 或 refs/tags/...）",
        ),
    )
    setup.add_argument(
        "--release-ref",
        metavar="REF",
        help=tr("refuse unreleased commits for this target ref", "拒绝这个目标 ref 上未发布的提交"),
    )
    setup.set_defaults(run=cmd_setup)

    gen = _command(
        verbs,
        "gen",
        tr(
            "generate User/xrobot_main.hpp for one config and select that config",
            "为一份配置生成 User/xrobot_main.hpp，并选中这份配置",
        ),
    )
    gen.add_argument(
        "-c",
        "--config",
        help=tr(
            "default: the selected config, else User/xrobot.yaml",
            "默认：选中的配置，没有时为 User/xrobot.yaml",
        ),
    )
    gen.set_defaults(run=cmd_gen)

    desc = _command(
        verbs,
        "describe",
        tr("print the BSP state as JSON for editors", "以 JSON 输出 BSP 的状态，供编辑器使用"),
    )
    desc.add_argument("-c", "--config", help=tr("default: the selected config", "默认：选中的配置"))
    desc.set_defaults(run=cmd_describe)

    for name, run, text in (
        (
            "sync",
            cmd_sync,
            tr(
                "add new fields and parameters with defaults, drop removed ones",
                "按默认值添加新的字段和参数，删除已移除的",
            ),
        ),
        ("format", cmd_format, tr("rewrite configs in the canonical layout", "按规范格式重写配置")),
    ):
        command = _command(verbs, name, text)
        command.add_argument(
            "-c",
            "--config",
            action="append",
            default=[],
            help=tr(
                "config to process; repeat for several (default: all)",
                "要处理的配置；多份时重复此选项（默认：全部）",
            ),
        )
        if name == "format":
            command.add_argument(
                "--check",
                action="store_true",
                help=tr("only report files that need formatting", "只报告需要格式化的文件"),
            )
        command.set_defaults(run=run)

    instance = _command(
        verbs,
        "instance",
        tr(
            "add, set, remove or rename one Module instance", "添加、修改、删除或重命名一个模块实例"
        ),
    )
    instance.add_argument(
        "-c",
        "--config",
        help=tr(
            "config to edit (default: the selected config)", "要编辑的配置（默认：选中的配置）"
        ),
    )
    actions = instance.add_subparsers(dest="action", required=True, metavar="<action>")
    add = _command(
        actions, "add", tr("add an instance with the default values", "按默认值添加一个实例")
    )
    add.add_argument("module", help=package_id)
    add.add_argument(
        "--id", help=tr("instance id (default: <module>_<n>)", "实例 id（默认：<模块>_<n>）")
    )
    change = _command(
        actions,
        "set",
        tr("replace one value", "替换一个值"),
        details=tr(
            "PATH is template_args[n] or args.<param>[.<field>|[n]]...; change an id\n"
            "with `xrobot instance rename`.\n"
            "VALUE is one YAML value, read like a value in the config: C++ code without quotes\n"
            "or in single quotes, a C++ string in double quotes (e.g. '\"bmi088_gyro\"').",
            "PATH 是 template_args[n] 或 args.<参数>[.<字段>|[n]]...；实例 id 用\n"
            "`xrobot instance rename` 修改。\n"
            "VALUE 是一个 YAML 值，读法与配置中的值相同：不加引号或用单引号的是 C++ 代码，\n"
            "双引号中的是 C++ 字符串（例如 '\"bmi088_gyro\"'）。",
        ),
    )
    change.add_argument("id", metavar="ID", help=instance_id)
    change.add_argument(
        "path",
        metavar="PATH",
        help=tr("value path, e.g. args.param.cycle", "值的路径，例如 args.param.cycle"),
    )
    change.add_argument("value", metavar="VALUE", help=tr("new value", "新的值"))
    change.add_argument(
        "--json",
        action="store_true",
        help=tr(
            "VALUE is JSON whose strings are C++ text", "VALUE 是 JSON，其中的字符串是 C++ 文本"
        ),
    )
    change.add_argument(
        "--if-match",
        metavar="SHA256",
        help=tr(
            "sha256 of the LF-normalized file the edit was based on",
            "编辑所依据的文件（换行统一为 LF 后）的 sha256",
        ),
    )
    _command(actions, "remove", tr("remove an instance", "删除一个实例")).add_argument(
        "id", help=instance_id
    )
    rename = _command(
        actions,
        "rename",
        tr(
            "rename an instance and the references to it in the config",
            "重命名一个实例，并更新配置中对它的引用",
        ),
    )
    rename.add_argument("id", help=instance_id)
    rename.add_argument("new_id", help=tr("new instance id", "新的实例 id"))
    instance.set_defaults(run=cmd_instance)

    module = _command(
        verbs,
        "module",
        tr(
            "add/remove a Module request, or show a Module interface",
            "添加或删除模块请求，或显示模块的接口",
        ),
    )
    module_actions = module.add_subparsers(dest="action", required=True, metavar="<action>")
    _command(
        module_actions,
        "add",
        tr("request a Module in modules.yaml", "在 modules.yaml 中请求一个模块"),
    ).add_argument("request", help="owner/Repo[@ref]")
    _command(
        module_actions, "remove", tr("remove a Module request", "删除一个模块请求")
    ).add_argument("request", help="owner/Repo")
    _command(
        module_actions,
        "show",
        tr("print the manifest and constructors of a Module", "输出模块的 manifest 和构造函数"),
    ).add_argument(
        "module",
        help=tr(
            "Module folder or header, or the id of a locked Module (owner/Repo or Repo)",
            "模块文件夹或头文件，或已锁定模块的 id（owner/Repo 或 Repo）",
        ),
    )
    module.set_defaults(run=cmd_module)

    new = _command(verbs, "new-module", tr("create a Module skeleton", "创建模块骨架"))
    new.add_argument(
        "name", help=tr("class and folder name (a C++ identifier)", "类名和文件夹名（C++ 标识符）")
    )
    new.add_argument(
        "--desc", metavar="TEXT", default="", help=tr("one-line description", "一句话说明")
    )
    new.add_argument(
        "--constructor",
        metavar="DECL",
        action="append",
        default=[],
        help=tr(
            "a C++ constructor parameter declaration; repeat for each parameter",
            "一个 C++ 构造参数声明；每个参数重复一次",
        ),
    )
    new.add_argument(
        "--template",
        metavar="DECL",
        action="append",
        default=[],
        help=tr(
            "a C++ template parameter declaration; repeat for each parameter",
            "一个 C++ 模板参数声明；每个参数重复一次",
        ),
    )
    new.add_argument(
        "--include",
        metavar="HEADER",
        action="append",
        default=[],
        help=tr(
            "another header to #include (libxr.hpp and the LibXR driver headers the "
            "declarations use are always included); repeat for each header",
            "另外要 #include 的头文件（libxr.hpp 和声明中用到的 LibXR 驱动头文件总会包含）；"
            "每个头文件重复一次",
        ),
    )
    new.add_argument(
        "--template-arg",
        metavar="ARG",
        action="append",
        default=[],
        help=tr(
            "a template argument the Module CI compiles with; repeat for each argument",
            "模块 CI 编译时使用的模板实参；每个实参重复一次",
        ),
    )
    new.add_argument(
        "--depends",
        metavar="MODULE",
        action="append",
        default=[],
        help=tr(
            "a Module this one depends on (owner/Repo[@ref]); repeat for each Module",
            "依赖的模块（owner/Repo[@ref]）；每个模块重复一次",
        ),
    )
    new.add_argument(
        "--out",
        metavar="DIR",
        default=".",
        help=tr("parent folder (default: .)", "上级文件夹（默认：.）"),
    )
    new.set_defaults(run=cmd_new_module)

    check = _command(
        verbs,
        "check-module",
        tr(
            "resolve Modules as setup does (updates xrobot.lock and Modules/), then write "
            "a never-executed constructor call for CI",
            "像 setup 一样解析模块（会更新 xrobot.lock 和 Modules/），然后写出一个供 CI 编译、"
            "从不执行的构造调用",
        ),
    )
    check.add_argument("module", help=package_id)
    check.add_argument(
        "-o",
        "--output",
        metavar="FILE",
        default="module_check.cpp",
        help=tr("default: %(default)s", "默认：%(default)s"),
    )
    check.add_argument(
        "--template-arg",
        metavar="ARG",
        action="append",
        default=[],
        help=tr("a template argument; repeat for each argument", "一个模板实参；每个实参重复一次"),
    )
    check.add_argument(
        "--offline",
        action="store_true",
        help=tr("use the Modules already in Modules/", "使用 Modules/ 中已有的模块"),
    )
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
    # 解析模块头文件会产生大量一直用到命令结束的对象，按默认阈值频繁回收只是白白扫描它们。
    # xrobot 是一次运行完就退出的命令，提高年轻代阈值后回收器仍在工作，只是很少触发。
    # Parsing Module headers creates many objects that live until the command ends, so
    # collecting at the default threshold only scans them again and again. xrobot runs
    # once and exits; with a higher young-generation threshold the collector still works,
    # just rarely.
    gc.set_threshold(100_000, 50, 100)
    args = parser().parse_args(argv)
    try:
        args.run(args)
    except (ConfigError, ProjectError, OSError, ValueError, yaml.YAMLError) as error:
        sys.stderr.write(f"{error}\n")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
