"""源：列出模块和 BSP 仓库的 index.yaml，以及选择这些源的 sources.yaml。
Sources: the index.yaml files that list Module and BSP repositories, and the
sources.yaml that selects them.
"""

import re
from pathlib import Path
from urllib.parse import urljoin

import requests
import yaml

DEFAULT_SOURCES = Path("Modules/sources.yaml")
DEFAULT_INDEX = Path("Modules/index.yaml")
_ID = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_.-]*/[A-Za-z0-9_][A-Za-z0-9_.-]*$")


def validate_id(identity):
    if (
        not isinstance(identity, str)
        or not _ID.fullmatch(identity)
        or any(p in (".", "..") for p in identity.split("/"))
    ):
        raise ValueError(f"Expected canonical owner/repo: {identity!r}")
    return identity


def extract_name_from_url(url):
    name = str(url).rstrip("/").rsplit("/", 1)[-1]
    return name[:-4] if name.endswith(".git") else name


def load_yaml(source):
    source = str(source)
    if source.startswith(("http://", "https://")):
        response = requests.get(source, timeout=20)
        response.raise_for_status()
        text = response.text
    else:
        text = Path(source).read_text(encoding="utf-8-sig")
    data = yaml.safe_load(text)
    if data is None:
        return {}
    if not isinstance(data, dict):
        raise ValueError(f"{source}: expected a YAML mapping")
    return data


def save_yaml(path, data):
    from xrobot.generate_main import atomic_write

    atomic_write(Path(path), yaml.safe_dump(data, sort_keys=False, allow_unicode=True))


def _relative(origin, value):
    value = str(value)
    if value.startswith(("https://", "http://", "file://", "git@", "ssh://")):
        return value
    if str(origin).startswith(("https://", "http://")):
        return urljoin(str(origin), value)
    return str((Path(origin).resolve().parent / value).resolve())


class ModuleSource:
    def __init__(self, url, public_key=None, priority=0):
        self.url, self.public_key, self.priority = str(url), public_key, int(priority)
        self.namespace = None
        self.mirror_of = None
        self.module_urls = []
        self.module_name_to_url = {}
        self.entries = {}
        self.index_data = {}

    def load_index(self):
        data = load_yaml(self.url)
        self.index_data = data
        self.namespace = data.get("namespace")
        self.mirror_of = data.get("mirror_of")
        self.entries.clear()
        for group, kind in (("modules", "module"), ("bsps", "bsp"), ("packages", None)):
            values = data.get(group, [])
            if not isinstance(values, list):
                raise ValueError(f"{self.url}: {group} must be a list")
            for value in values:
                record = {"repo": value} if isinstance(value, str) else dict(value)
                package_type = record.get("type", kind)
                if package_type not in ("module", "bsp") or (kind and package_type != kind):
                    raise ValueError("Catalog entry requires type module or bsp")
                repo = record.get("repo", record.get("source"))
                if not isinstance(repo, str) or not repo:
                    raise ValueError("Catalog entry is missing repository URL")
                identity = record.get("id")
                if identity is None:
                    match = re.search(r"github\.com[/:]([^/]+/[^/]+?)(?:\.git)?/?$", repo, re.I)
                    if self.mirror_of:
                        identity = str(self.mirror_of) + "/" + extract_name_from_url(repo)
                    elif match:
                        identity = match.group(1)
                    elif self.namespace:
                        identity = str(self.namespace) + "/" + extract_name_from_url(repo)
                validate_id(identity)
                if package_type == "bsp":
                    # The Git repository already identifies the BSP. Do not make
                    # platform, MCU, build or validation labels a use requirement.
                    record = {
                        "id": identity,
                        "type": "bsp",
                        "repo": _relative(self.url, repo),
                        "source": self.url,
                    }
                else:
                    status = record.get("status", "community")
                    if status not in ("community", "verified", "official"):
                        raise ValueError(f"Unknown package status: {status}")
                    if status in ("verified", "official") and not (
                        record.get("tested_ref") and record.get("tested_libxr")
                    ):
                        raise ValueError(
                            f"{identity}: validation label needs tested_ref and tested_libxr"
                        )
                    for field in ("tested_ref", "tested_libxr", "tested_xrobot"):
                        if field in record:
                            record[field] = str(record[field])
                    record.update(
                        id=identity,
                        type=package_type,
                        repo=_relative(self.url, repo),
                        source=self.url,
                        status=status,
                    )
                if identity.casefold() in {i.casefold() for i in self.entries}:
                    raise ValueError(f"Duplicate catalog identity: {identity}")
                self.entries[identity] = record
        self.module_urls = [r["repo"] for r in self.entries.values() if r["type"] == "module"]
        self.module_name_to_url = {
            i: r["repo"] for i, r in self.entries.items() if r["type"] == "module"
        }
        return True

    def add_module_url(self, repo_url):
        values = self.index_data.setdefault("modules", [])
        if repo_url not in values:
            values.append(repo_url)

    def save_index_yaml(self, path=None):
        target = path or self.url
        if str(target).startswith(("http://", "https://")):
            raise ValueError("Saving a remote index requires a local output path")
        save_yaml(target, self.index_data)


def get_primary_namespace(source):
    return source.mirror_of or source.namespace


class SourceManager:
    def __init__(self, sources_yaml=DEFAULT_SOURCES):
        self.sources = []
        self.packages = {}
        self.module_map = {}
        self.module_source_map = {}
        self.all_module_candidates = {}
        if str(sources_yaml).startswith(("https://", "http://")) or Path(sources_yaml).exists():
            self.load_sources(sources_yaml)

    def load_sources(self, path):
        data = load_yaml(path)
        sources = data.get("sources", [])
        if not isinstance(sources, list):
            raise ValueError("sources must be a list")
        self.sources, self.packages = [], {}
        self.module_map, self.module_source_map, self.all_module_candidates = {}, {}, {}
        for entry in sources:
            source = ModuleSource(
                _relative(path, entry["url"]), entry.get("public_key"), entry.get("priority", 0)
            )
            source.load_index()
            self.sources.append(source)
        self.sources.sort(key=lambda item: item.priority)
        normalized = {}
        selected_priorities = {}
        for source in self.sources:
            for identity, record in source.entries.items():
                key = normalized.setdefault(identity.casefold(), identity)
                self.all_module_candidates.setdefault(key, []).append((record["repo"], source))
                if key in self.packages:
                    if (
                        selected_priorities[key] == source.priority
                        and self.packages[key]["repo"] != record["repo"]
                    ):
                        raise ValueError(
                            f"Equal-priority sources disagree about {key}; set source priorities explicitly"
                        )
                    continue
                self.packages[key] = dict(record)
                selected_priorities[key] = source.priority
                self.module_source_map[key] = source
                if record["type"] == "module":
                    self.module_map[key] = record["repo"]
        # xrobot.lock records the canonical repository; mirror catalogs only change
        # where sources are fetched from.
        for key, record in self.packages.items():
            canonical = [
                repo
                for repo, source in self.all_module_candidates.get(key, [])
                if not source.mirror_of
            ]
            record["canonical"] = canonical[0] if canonical else record["repo"]

    def resolve_id(self, name, kind=None):
        candidates = [
            identity
            for identity, record in self.packages.items()
            if (not kind or record["type"] == kind)
            and (
                identity.casefold() == name.casefold()
                if "/" in name
                else identity.rsplit("/", 1)[-1] == name
            )
        ]
        if not candidates:
            raise ValueError(f"Package not found: {name}")
        if len(candidates) != 1:
            raise ValueError(f"Ambiguous package {name}; specify {', '.join(sorted(candidates))}")
        return candidates[0]

    def list_modules(self):
        return sorted(self.module_map)

    def get_repo_url(self, identity):
        return self.packages[self.resolve_id(identity)]["repo"]

    def find_module(self, identity):
        return self.all_module_candidates[self.resolve_id(identity)]

    def save_sources_yaml(self, path=DEFAULT_SOURCES):
        save_yaml(path, {"sources": [{"url": s.url, "priority": s.priority} for s in self.sources]})

    def save_index_yaml(self, source, path=None):
        source.save_index_yaml(path)


def add_source(sources_yaml, url, public_key=None, priority=0):
    """向 sources.yaml 添加一个源；已列出的地址不重复添加。
    Add a Source to sources.yaml; a URL already listed is not added again.
    """
    data = load_yaml(sources_yaml) if Path(sources_yaml).exists() else {"sources": []}
    if not any(item["url"] == url for item in data["sources"]):
        entry = {"url": url, "priority": int(priority)}
        if public_key:
            entry["public_key"] = public_key
        data["sources"].append(entry)
        save_yaml(sources_yaml, data)


def create_sources_yaml(path=DEFAULT_SOURCES):
    """写入只含官方源的 sources.yaml。
    Write a sources.yaml that lists the official Source only.
    """
    save_yaml(
        path,
        {"sources": [{"url": "https://xrobot.work/xrobot-modules/index.yaml", "priority": 0}]},
    )


def create_index_yaml(path=DEFAULT_INDEX, namespace="local", mirror_of=None):
    """写入新的 index.yaml，其中以 BlinkLED 作为示例条目。
    Write a new index.yaml with BlinkLED as an example entry.
    """
    data = {
        "namespace": namespace,
        "modules": ["https://github.com/xrobot-org/BlinkLED.git"],
        "bsps": [],
    }
    if mirror_of:
        data["mirror_of"] = mirror_of
    save_yaml(path, data)


def add_index_entry(index_yaml, repo_url):
    """向 index.yaml 添加一个模块仓库；文件不存在时新建。
    Add a Module repository to index.yaml, creating the file when it does not exist.
    """
    data = (
        load_yaml(index_yaml)
        if Path(index_yaml).exists()
        else {"namespace": "local", "modules": []}
    )
    if repo_url not in data.setdefault("modules", []):
        data["modules"].append(repo_url)
        save_yaml(index_yaml, data)
