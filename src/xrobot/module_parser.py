"""模块的 manifest、构造函数接口，以及按 xrobot.lock 找到已检出的模块。
Module manifests, constructor interfaces, and the checked-out Modules that xrobot.lock
lists.
"""

import copy
import difflib
import json
import re
from pathlib import Path

import yaml

from xrobot import __version__
from xrobot.constructor_model import enrich_interface
from xrobot.git import git
from xrobot.source_syntax import extract_interface

MANIFEST_PATTERN = re.compile(
    r"/\*\s*=== MODULE MANIFEST(?: V(\d+))? ===\s*(.*?)\s*=== END MANIFEST ===\s*\*/", re.S
)
# 本版本读取的 manifest 格式；声明更新格式的模块需要更新的 xrobot。
# The manifest format this version reads; a Module that declares a newer one needs a
# newer xrobot.
MANIFEST_VERSION = 2
MANIFEST_KEYS = ("module_description", "depends", "standalone")


class ModuleManifest:
    """模块头文件中 MODULE MANIFEST V2 块的内容。
    The content of the MODULE MANIFEST V2 block of a Module header.
    """

    def __init__(self, manifest: dict, path: Path | None = None) -> None:
        self.manifest = manifest
        self.path = path

    @property
    def description(self) -> str:
        """模块说明。
        The Module description.
        """
        return self.manifest.get("module_description", "")

    @property
    def depends(self) -> list:
        """依赖请求，写法与 modules.yaml 相同。
        The dependency requests, written as in modules.yaml.
        """
        return self.manifest.get("depends", [])

    @property
    def standalone(self) -> bool:
        """是否可在配置中建实例；为 False 时是只提供源码的库。
        Whether configurations can instantiate it; False for a library that only provides
        sources.
        """
        return self.manifest.get("standalone", True) is not False

    def as_dict(self) -> dict:
        """manifest 的原始映射。
        The manifest as its mapping.
        """
        return dict(self.manifest)


def manifest_from_text(text: str, path: str | Path | None = None) -> ModuleManifest:
    """从头文件文本中读取 MODULE MANIFEST V2。
    Read the MODULE MANIFEST V2 from the text of a header.

    Raises:
        ValueError: 没有 manifest、有多个、格式早于 1.0 或新于本版本，或内容不合法。
            No manifest, several, a format from before 1.0 or newer than this version, or
            invalid content.
    """
    matches = list(MANIFEST_PATTERN.finditer(text))
    if not matches:
        raise ValueError(f"{path}: no MODULE MANIFEST V{MANIFEST_VERSION} block")
    if len(matches) > 1:
        raise ValueError(f"{path}: multiple package manifests")
    version = int(matches[0].group(1) or 1)
    if version > MANIFEST_VERSION:
        raise ValueError(
            f"{path}: MODULE MANIFEST V{version} needs a newer xrobot; "
            f"xrobot {__version__} reads manifests up to V{MANIFEST_VERSION}"
        )
    if version < MANIFEST_VERSION:
        raise ValueError(
            f"{path}: this MODULE MANIFEST predates XRobot 1.0; update the Module to "
            f"MODULE MANIFEST V{MANIFEST_VERSION} with {', '.join(MANIFEST_KEYS)} (the C++ "
            "constructor is the interface)"
        )
    data = yaml.safe_load(matches[0].group(2))
    if data is None:
        data = {}
    if not isinstance(data, dict):
        raise ValueError(f"{path}: package manifest must be a mapping")
    unknown = [k for k in data if k not in MANIFEST_KEYS]
    if unknown:
        raise ValueError(
            f"{path}: unsupported manifest key(s) {', '.join(map(str, unknown))}; "
            f"MODULE MANIFEST V{MANIFEST_VERSION} holds only {', '.join(MANIFEST_KEYS)}"
        )
    if not isinstance(data.get("depends", []), list):
        raise ValueError(f"{path}: depends must be a list")
    return ModuleManifest(data, path)


def parse_manifest_from_header(header_path: str | Path) -> ModuleManifest:
    """读取一个头文件的 manifest。
    Read the manifest of a header file.
    """
    header_path = Path(header_path)
    return manifest_from_text(header_path.read_text(encoding="utf-8-sig"), header_path)


def load_single_module(path: str | Path) -> ModuleManifest:
    """读取模块目录（其中的 <目录名>.hpp）或头文件的 manifest。
    Read the manifest of a Module folder (its <folder>.hpp) or of a header.
    """
    path = Path(path).resolve()
    if path.is_dir():
        path = path / (path.name + ".hpp")
    return parse_manifest_from_header(path)


_INTERFACE_CACHE = {}


def source_interface(path: str | Path) -> dict:
    """模块的构造函数接口（模板参数与公开构造函数），按文件的修改时间和大小缓存。
    The constructor interface of a Module (template parameters and public constructors),
    cached per file modification time and size.

    Raises:
        ValueError: 头文件无法解析。
            The header cannot be parsed.
    """
    path = Path(path)
    if path.is_dir():
        path = path / (path.name + ".hpp")
    stat = path.stat()
    key = (str(path.resolve()), stat.st_mtime_ns, stat.st_size)
    if key not in _INTERFACE_CACHE:
        try:
            # 与 TypeIndex 读取同样的文本，两者共用一次解析。
            # Same text as TypeIndex reads, so both share one parsed document.
            source = path.read_text(encoding="utf-8", errors="surrogateescape")
            name = str(path.resolve())
            _INTERFACE_CACHE[key] = enrich_interface(
                source, extract_interface(source, path.stem, source_name=name), name
            )
        except ValueError as error:
            raise ValueError(f"{path}: {error}") from error
    return copy.deepcopy(_INTERFACE_CACHE[key])


def _locked_head(folder: Path) -> str:
    """模块检出的 HEAD commit。
    The HEAD commit of a Module checkout.

    Raises:
        ValueError: git 读不出 HEAD。
            git cannot read HEAD.
    """
    try:
        return git(folder, "rev-parse", "--verify", "HEAD")
    except ValueError as error:
        raise ValueError(f"cannot read the commit of {folder}: {error}") from error


def _module_record(identity: str, folder: Path) -> dict:
    """一个已检出模块的记录：id、类名、目录、主头文件和 manifest。
    The record of a checked-out Module: id, class name, folder, main header and manifest.
    """
    header = folder / (folder.name + ".hpp")
    return {
        "id": identity,
        "name": folder.name,
        "path": folder,
        "header": header,
        "manifest": parse_manifest_from_header(header),
    }


def locked_modules(directory: Path, lock_path: Path) -> list[dict]:
    """xrobot.lock 中每个模块的状态：ok、missing、mismatch 或 broken（附原因）。
    The state of every xrobot.lock entry: ok, missing, mismatch or broken (with a reason).

    Raises:
        ValueError: lock 中的 id 会让路径离开 Modules/。
            An id in the lock would lead out of Modules/.
    """
    directory = Path(directory)
    lock = yaml.safe_load(Path(lock_path).read_text(encoding="utf-8")) or {}
    states = []
    for identity, record in (lock.get("modules") or {}).items():
        folder = (directory / identity).resolve()
        if directory.resolve() not in folder.parents:
            raise ValueError(f"Module path leaves directory: {identity}")
        commit = (record or {}).get("commit")
        state = {
            "id": identity,
            "folder": folder,
            "commit": commit,
            "head": None,
            "status": "ok",
            "reason": None,
        }
        if not re.fullmatch(r"[0-9a-f]{40}", str(commit or "")):
            state.update(status="broken", reason=f"xrobot.lock has no commit for {identity}")
        elif not (folder / (folder.name + ".hpp")).is_file():
            state["status"] = "missing"
        elif not (folder / ".git").exists():
            state.update(
                status="broken",
                reason=f"Modules/{identity} is not a Git checkout; delete it and run "
                "`xrobot setup`",
            )
        else:
            try:
                state["head"] = _locked_head(folder)
            except ValueError as error:
                state.update(status="broken", reason=str(error))
            else:
                if state["head"] != commit:
                    state["status"] = "mismatch"
        states.append(state)
    return states


def lock_error(state: dict) -> str:
    """一个非 ok 状态的报错信息，说明怎么恢复。
    The error message for a state that is not ok, with how to recover.
    """
    if state["status"] == "broken":
        return state["reason"]
    if state["status"] == "missing":
        return f"{state['id']} from xrobot.lock is not checked out; run `xrobot setup`"
    return (
        f"{state['id']} is checked out at {state['head'][:12]} but xrobot.lock pins "
        f"{state['commit'][:12]}. While developing a module, keep your changes uncommitted; "
        "when they are ready, push them to a branch of the module and run "
        f"`xrobot setup --update {state['id']}`. To return to the locked sources run "
        "`xrobot setup`."
    )


def discover_modules(directory: Path, lock_path: str | Path) -> dict[str, dict]:
    """xrobot.lock 中各模块的 id 与本地记录。
    The ids and local records of the Modules in xrobot.lock.

    lock 是唯一依据：未列出的目录（旧缓存、手动克隆）被忽略；每个列出的模块都必须检出在
    锁定的 commit，这样读到的接口正是构建所编译的源码。
    The lock is the only source of truth: unlisted folders (stale caches, manual clones)
    are ignored, and every listed Module must be checked out at its locked commit, so the
    interfaces come from the sources the build compiles.

    Raises:
        ValueError: lock 不存在，或有模块缺失、不在锁定的 commit、或损坏。
            The lock does not exist, or a Module is missing, not at its locked commit, or
            broken.
    """
    lock_path = Path(lock_path)
    if not lock_path.is_file():
        raise ValueError(
            f"{lock_path.name} does not exist; run `xrobot setup` to resolve the Modules"
        )
    result = {}
    problems = []
    for state in locked_modules(directory, lock_path):
        if state["status"] != "ok":
            problems.append(lock_error(state))
            continue
        result[state["id"]] = _module_record(state["id"], state["folder"])
    if problems:
        raise ValueError("\n".join(problems))
    return result


def select_module(modules: dict[str, dict], requested: str) -> dict:
    """按 owner/Repo 或 Repo 选出模块，不区分大小写。
    Pick a Module by owner/Repo or Repo, ignoring case.

    Raises:
        ValueError: 找不到（附最接近的候选），或只写 Repo 时有多个同名模块。
            Not found (with the closest candidates), or several Modules share the Repo
            name.
    """
    candidates = [
        value
        for key, value in modules.items()
        if (
            key.casefold() == requested.casefold()
            if "/" in requested
            else value["name"].casefold() == requested.casefold()
        )
    ]
    if not candidates:
        names = {
            (v["id"] if "/" in requested else v["name"]).casefold(): v["id"]
            for v in modules.values()
        }
        close = difflib.get_close_matches(requested.casefold(), list(names), n=3)
        hint = f"; did you mean {', '.join(names[c] for c in close)}?" if close else ""
        raise ValueError(f"Module not found: {requested}{hint}")
    if len(candidates) != 1:
        raise ValueError(
            f"Ambiguous Module {requested}; specify {', '.join(v['id'] for v in candidates)}"
        )
    return candidates[0]


def print_manifest(manifest: ModuleManifest) -> None:
    """输出 manifest；可建实例的模块还输出模板参数和构造函数。
    Print the manifest, and for an instantiable Module its template parameters and
    constructors.
    """
    print(json.dumps(manifest.as_dict(), ensure_ascii=False, indent=2))
    if manifest.standalone and manifest.path:
        interface = source_interface(manifest.path)
        if interface["template"] is not None:
            print(f"template <{interface['template']}>")
        for declaration in interface["constructors"]:
            print(f"{manifest.path}:{declaration['line']}: {declaration['declaration']}")
