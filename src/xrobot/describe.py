"""Describe a BSP's XRobot state as JSON for editors; reads only, never writes.

The output is what ``xrobot gen`` reads and enforces: application configs and
the selected product, the generated header's freshness, tool pins, locked
Module sources, constructor signatures and the struct fields / constructors a
YAML mapping must name, XR_REGISTER registrations with their types, binding
candidates per instance, config constants and generation diagnostics. Editors
render and edit from it without parsing C++ or manifests themselves.
"""

from pathlib import Path

from xrobot import __version__
from xrobot.config import load_config
from xrobot.constructor_model import (
    compliant_constructors,
    initializer_tree,
    is_dependency,
    qualify,
    type_shape,
)
from xrobot.generate_main import generate_code, read_registrations
from xrobot.lock import read_modules_yaml
from xrobot.module_parser import (
    _module_record,
    lock_error,
    locked_modules,
    select_module,
    source_interface,
)
from xrobot.project import ProjectError
from xrobot.type_index import TypeIndex

SCHEMA = 1


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

    def _member(self, name, cpp_type, entry):
        return {"name": name, "type": cpp_type, "type_ref": self.ref(cpp_type, entry.path)}


def _matches(index, source, target):
    """Whether an object of ``source`` binds to ``target`` (exact, or a located public base)."""
    sb, tb = type_shape(source)[0], type_shape(target)[0]
    if sb == tb:
        return True
    try:
        entry, wanted = index.resolve(source), index.resolve(target)
    except ValueError:
        return False
    if entry is None or wanted is None:
        return False
    pending, seen = [entry], set()
    while pending:
        current = pending.pop()
        if current.path == wanted.path:
            return True
        if current.path in seen:
            continue
        seen.add(current.path)
        for access, base in current.base_spellings():
            if access == "public":
                parent = index.resolve(base, current.path[:-1])
                if parent is not None:
                    pending.append(parent)
    return False


def _describe_module(module, project, types, registrations):
    result = {
        "id": module["id"],
        "class": module["name"],
        "header": project.relative(module["header"]),
        "standalone": bool(module["manifest"].standalone),
    }
    if not result["standalone"]:
        return result
    interface = source_interface(module["header"])
    cpp_class = module["name"]
    result["template_parameters"] = [
        {"name": p["name"], "type": p["type"], "default": p["default"]}
        for p in interface["template_parameters"]
    ]
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
                    "candidates": [
                        r["name"] for r in registrations if _matches(types.index, r["type"], target)
                    ],
                }
            )
        constructors.append({"line": ctor.get("line"), "parameters": parameters})
    result["constructors"] = constructors
    return result


def describe(project, config=None):
    diagnostics = []

    def report(severity, scope, message):
        for line in str(message).splitlines():
            diagnostics.append({"severity": severity, "scope": scope, "message": line})

    try:
        selected = project.selected_config()
    except ProjectError as error:
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
                modules[state["id"]] = _module_record(state["id"], state["folder"])
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
            f"generated from older inputs ({', '.join(header['newer'] + header['missing'])}); run `xrobot gen`",
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
        earlier = []
        for item in config_data.get("modules", []):
            try:
                module = select_module(modules, item["module"])
            except ValueError:
                module = None  # reported by the generation diagnostic below
            candidates = {}
            if module is not None and described.get(module["id"], {}).get("constructors"):
                for ctor in described[module["id"]]["constructors"]:
                    for p in ctor["parameters"]:
                        names = list(p["candidates"])
                        names += [
                            i for i, cls in earlier if cls and _matches(index, cls, p["type"])
                        ]
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
            earlier.append((item["id"], module["name"] if module else None))
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
