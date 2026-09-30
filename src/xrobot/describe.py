"""以 JSON 描述 BSP 的 XRobot 状态，供编辑器使用；只读，不写任何文件。
Describe a BSP's XRobot state as JSON for editors; reads only, never writes.

输出的是 `xrobot gen` 读取和检查的全部内容：应用配置和当前产品、生成头文件是否过期、工具
版本、锁定的模块、构造函数签名、映射须写出的字段或构造参数、XR_REGISTER 注册及其类型、每个
实例参数可绑定的名字、配置中的常量，以及生成时的错误。编辑器据此显示和编辑，不必自己解析
C++ 或 manifest。
The output is what `xrobot gen` reads and enforces: application configs and the selected
product, the generated header's freshness, tool pins, locked Module sources, constructor
signatures and the fields or constructor parameters a YAML mapping must name,
XR_REGISTER registrations with their types, binding candidates per instance parameter,
config constants and generation diagnostics. Editors render and edit from it without
parsing C++ or manifests themselves.
"""

from __future__ import annotations

from pathlib import Path

from xrobot import __version__
from xrobot.config import load_config
from xrobot.constructor_model import (
    binding_candidates,
    compliant_constructors,
    initializer_tree,
    is_dependency,
    qualify,
    type_shape,
)
from xrobot.generate_main import generate_code, read_registrations
from xrobot.lock import read_modules_yaml
from xrobot.module_parser import (
    lock_error,
    locked_modules,
    module_interface,
    module_record,
    select_module,
)
from xrobot.project import Project, ProjectError
from xrobot.type_index import ClassEntry, TypeIndex

SCHEMA = 1


class _Types:
    """收集构造参数能到达的每个类型的映射形状，输出为 describe 的 types。
    Collect the mapping shape of every type reachable from a constructor parameter, output
    as the types of describe.
    """

    def __init__(self, index: TypeIndex) -> None:
        self.index = index
        self.table: dict[str, dict | None] = {}

    def ref(self, cpp_type: str | None, scope: tuple[str, ...] = ()) -> str | None:
        """类型在 types 中的键（全名）；不在索引中时为 None。首次遇到时登记其形状。
        The key (qualified name) of a type in types; None outside the index. Its shape is
        registered the first time it is met.

        形状是 aggregate（字段）、class（构造函数）或 opaque（映射无法检查的原因）。
        A shape is aggregate (fields), class (constructors) or opaque (why a mapping cannot
        be checked).
        """
        try:
            entry = self.index.resolve(cpp_type, scope) if cpp_type else None
        except ValueError:
            return None
        if entry is None:
            return None
        key = entry.qualified
        if key not in self.table:
            self.table[key] = None  # reserve before recursing into member types
            problem = entry.mapping_problem()
            if problem:
                self.table[key] = {"kind": "opaque", "reason": problem}
            elif entry.is_aggregate():
                defaults = entry.layout().field_defaults
                self.table[key] = {
                    "kind": "aggregate",
                    "fields": [
                        dict(
                            self._member(name, self.index.qualify_in(typ, entry, key), entry),
                            default=defaults.get(name),
                        )
                        for name, typ, _ in entry.fields()
                    ],
                }
            else:
                self.table[key] = {
                    "kind": "class",
                    "constructors": [
                        [
                            dict(
                                self._member(
                                    p["name"], self.index.qualify_in(p["type"], entry, key), entry
                                ),
                                default=p["default"],
                            )
                            for p in ctor
                        ]
                        for ctor in entry.constructors()
                        if ctor
                    ],
                }
        return key

    def _member(self, name: str, cpp_type: str, entry: ClassEntry) -> dict:
        """一个字段或构造参数：名字、类型和类型在 types 中的键。
        One field or constructor parameter: name, type and the type's key in types.
        """
        return {"name": name, "type": cpp_type, "type_ref": self.ref(cpp_type, entry.path)}


def _describe_module(
    module: dict, project: Project, types: _Types, registrations: list[dict]
) -> dict:
    """一个模块的描述：类名、头文件、是否可实例化，以及模板参数和构造函数。
    The description of one Module: class, header, whether it can be instantiated, and its
    template parameters and constructors.

    每个构造参数带类型、默认值、是否依赖参数、默认值中的字段、类型在 types 中的键，以及
    可以写的注册名。
    Each constructor parameter carries its type, default, whether it is a dependency, the
    fields of its default, its type's key in types and the registrations it can take.

    Raises:
        ValueError: 头文件无法解析。
            The header cannot be parsed.
    """
    result = {
        "id": module["id"],
        "class": module["name"],
        "header": project.relative(module["header"]),
        "standalone": bool(module["manifest"].standalone),
    }
    if not result["standalone"]:
        return result
    interface = module_interface(module)
    cpp_class = module["name"]
    result["template_parameters"] = [
        {"name": p["name"], "type": p["type"], "default": p["default"]}
        for p in interface["template_parameters"]
    ]
    registered = {r["name"]: r["type"] for r in registrations}
    constructors = []
    for ctor in compliant_constructors(interface):
        parameters = []
        for p in ctor["arguments"]:
            target = qualify(p["type"], interface, cpp_class, None, True)
            default = qualify(p["default"], interface, cpp_class)
            try:
                tree = initializer_tree(default, target)
            except ValueError:
                tree = default
            parameters.append(
                {
                    "name": p["name"],
                    "type": target,
                    "default": default,
                    "dependency": is_dependency(p),
                    "default_fields": tree if isinstance(tree, dict) else None,
                    "type_ref": types.ref(target),
                    "candidates": binding_candidates(
                        types.index, target, registered, is_dependency(p)
                    ),
                }
            )
        constructors.append({"line": ctor.get("line"), "parameters": parameters})
    result["constructors"] = constructors
    return result


def describe(project: Project, config: str | Path | None = None) -> dict:
    """描述 BSP：config（缺省为当前选中的产品）及生成它所需的一切。
    Describe the BSP: config (default: the selected product) and everything its generation
    needs.

    出错的部分写入 diagnostics，其余照常输出，编辑器始终能拿到可用的内容。
    Problems go to diagnostics and the rest is still described, so an editor always gets
    something usable.

    Returns:
        可直接序列化为 JSON 的映射；schema 为 SCHEMA。
        A mapping ready to be written as JSON; its schema is SCHEMA.
    """
    diagnostics = []

    def report(severity: str, scope: str, message: object) -> None:
        # gen 的报错以文件开头；scope 已经是这个文件，不在 message 中重复。
        # gen's messages start with the file; scope already names it, so message drops it.
        prefix = scope + ": "
        for line in str(message).splitlines():
            line = line.removeprefix(prefix)
            diagnostics.append({"severity": severity, "scope": scope, "message": line})

    try:
        selected = project.selected_config()
    except ProjectError as error:
        # 编辑器仍需要其他配置才能选一个。
        # Editors still need the other configurations to select one.
        selected = project.header_selection()
        report("error", project.relative(project.header), error)
    config_path = Path(config) if config else selected

    tools = {"xrobot": {"installed": __version__, "pin": None}}
    try:
        _, tools["xrobot"]["pin"] = read_modules_yaml(project.modules_yaml)
    except ValueError as error:
        report("error", "Modules/modules.yaml", error)
    pin = tools["xrobot"]["pin"]
    if pin is None:
        report(
            "warning", "Modules/modules.yaml", f"XRobot is not pinned; add `xrobot: {__version__}`"
        )
    elif pin != __version__:
        report(
            "warning",
            "Modules/modules.yaml",
            f"installed XRobot {__version__} differs from the pinned {pin}",
        )

    lock_info = {
        "path": project.relative(project.lock),
        "present": project.lock.is_file(),
        "modules": [],
    }
    modules = {}
    if project.lock.is_file():
        for state in locked_modules(project.modules_dir, project.lock):
            lock_info["modules"].append({k: state[k] for k in ("id", "commit", "head", "status")})
            if state["status"] == "ok":
                modules[state["id"]] = module_record(state["id"], state["folder"])
            else:
                report("error", state["id"], lock_error(state))
        statuses = {m["status"] for m in lock_info["modules"]}
        lock_info["status"] = "ok" if statuses <= {"ok"} else sorted(statuses - {"ok"})[0]
    else:
        lock_info["status"] = "absent"
        report("error", "xrobot.lock", "xrobot.lock does not exist; run `xrobot setup`")

    header = project.header_state()
    if header["status"] == "stale":
        report(
            "warning",
            header["path"],
            f"generated from older inputs ({', '.join(header['newer'] + header['missing'])}); "
            "run `xrobot gen`",
        )
    elif header["status"] in ("missing", "unreadable"):
        report("warning", header["path"], "not generated; run `xrobot setup`")

    records = []
    entry_path = None
    try:
        entry_path = project.entry()
        records = read_registrations(entry_path)
    except (OSError, ValueError) as error:
        report("error", "registrations", error)
    registrations = [{"name": r["name"], "type": r["type"]} for r in records]

    index = TypeIndex.for_modules(modules)
    types = _Types(index)
    described = {}
    for identity, module in sorted(modules.items()):
        try:
            described[identity] = _describe_module(module, project, types, registrations)
        except (OSError, ValueError) as error:
            described[identity] = {"id": identity, "class": module["name"], "error": str(error)}
            report("error", identity, error)

    source = project.relative(config_path)
    config_data, instances, constants = None, [], []
    try:
        config_data = load_config(config_path, source)
    except (OSError, ValueError) as error:
        report("error", source, error)
    if config_data is not None:
        namespace = config_data.get("constexpr_namespace", "ProjectConstexpr")
        constants = [
            {"name": name, "qualified": namespace + "::" + name, "type": spec["type"]}
            for name, spec in config_data.get("constexprs", {}).items()
        ]
        # 可绑定的名字：注册名，以及排在前面的实例（按顺序构造，只能引用前面的）。
        # Names that can bind: registrations, and earlier instances (instances are
        # constructed in order, so only earlier ones can be referenced).
        named = {r["name"]: r["type"] for r in registrations}
        for item in config_data.get("modules", []):
            try:
                module = select_module(modules, item["module"])
            except ValueError:
                module = None  # reported by the generation diagnostic below
            candidates = {}
            if module is not None and described.get(module["id"], {}).get("constructors"):
                for ctor in described[module["id"]]["constructors"]:
                    for p in ctor["parameters"]:
                        names = binding_candidates(index, p["type"], named, p["dependency"])
                        names += [
                            c["qualified"]
                            for c in constants
                            if type_shape(c["type"])[0] == type_shape(p["type"])[0]
                        ]
                        candidates.setdefault(p["name"], [])
                        candidates[p["name"]] += [
                            n for n in names if n not in candidates[p["name"]]
                        ]
            instances.append(
                {
                    "id": item["id"],
                    "module": module["id"] if module else item["module"],
                    "class": module["name"] if module else None,
                    "template_args": item.get("template_args", []),
                    "args": item.get("args", []),
                    "candidates": candidates,
                }
            )
            if module is not None:
                named[item["id"]] = module["name"]
        if lock_info["status"] == "ok" and entry_path is not None:
            try:
                generate_code(project, config_path, modules, records, index)
            except (OSError, ValueError) as error:
                report("error", source, error)

    return {
        "schema": SCHEMA,
        "root": project.root.as_posix(),
        "configs": [project.relative(p) for p in project.configs()],
        "config": source,
        "selected": project.relative(selected),
        "header": header,
        "tools": tools,
        "lock": lock_info,
        "entry": project.relative(entry_path) if entry_path else None,
        "registrations": registrations,
        "modules": described,
        "types": types.table,
        "instances": instances,
        "constexprs": constants,
        "diagnostics": diagnostics,
    }
