"""源：列出模块和 BSP 仓库的 index.yaml，以及选择这些源的 sources.yaml。
Sources: the index.yaml files that list Module and BSP repositories, and the
sources.yaml that selects them.

xrobot setup 用这里的包表把 owner/Repo 解析为仓库地址，xrobot source 用它查询和编辑源。
xrobot setup maps owner/Repo to a repository with the package table built here, and
xrobot source queries and edits the Sources with it.
"""

import re
from pathlib import Path
from urllib.parse import urljoin

import requests
import yaml

OFFICIAL_SOURCE = "https://xrobot.work/xrobot-modules/index.yaml"
SOURCES_TEMPLATE = f"sources:\n  - url: {OFFICIAL_SOURCE}\n    priority: 0\n"
STATUSES = ("community", "verified", "official")
FIELDS = (
    "id",
    "type",
    "repo",
    "canonical",
    "source",
    "status",
    "tested_ref",
    "tested_libxr",
    "tested_xrobot",
)
_ID = re.compile(r"^[A-Za-z0-9_][A-Za-z0-9_.-]*/[A-Za-z0-9_][A-Za-z0-9_.-]*$")
_GITHUB = re.compile(r"github\.com[/:]([^/]+/[^/]+?)(?:\.git)?/?$", re.I)


class SourceUnavailable(ValueError):
    """源的 index.yaml 无法下载。
    A Source's index.yaml could not be downloaded.
    """


def validate_id(identity: object) -> str:
    """检查包 id 是否为规范的 owner/Repo，并原样返回。
    Check that a package id is a canonical owner/Repo and return it.

    Raises:
        ValueError: 不是 owner/Repo，或含有 . 或 .. 路径段。
            Not owner/Repo, or a . or .. path segment.
    """
    if (
        not isinstance(identity, str)
        or not _ID.fullmatch(identity)
        or any(p in (".", "..") for p in identity.split("/"))
    ):
        raise ValueError(f"Expected canonical owner/repo: {identity!r}")
    return identity


def extract_name_from_url(url: str) -> str:
    """仓库地址的最后一段，去掉 .git。
    The last segment of a repository URL, without .git.
    """
    name = str(url).rstrip("/").rsplit("/", 1)[-1]
    return name[:-4] if name.endswith(".git") else name


def _reason(error: requests.RequestException) -> str:
    """下载失败的简短原因。
    A short reason for a failed download.
    """
    if isinstance(error, requests.HTTPError) and error.response is not None:
        return f"HTTP {error.response.status_code}"
    if isinstance(error, requests.Timeout):
        return "timed out"
    if isinstance(error, requests.ConnectionError):
        return "cannot connect"
    return type(error).__name__


def load_yaml(source: str | Path) -> dict:
    """读取本地文件或 HTTP(S) 地址上的 YAML 映射；空文档为空映射。
    Read a YAML mapping from a local file or an HTTP(S) URL; an empty document is {}.

    Raises:
        SourceUnavailable: 地址无法下载。
            The URL could not be downloaded.
        ValueError: 内容不是映射。
            The document is not a mapping.
    """
    source = str(source)
    if source.startswith(("http://", "https://")):
        try:
            response = requests.get(source, timeout=20)
            response.raise_for_status()
        except requests.RequestException as error:
            raise SourceUnavailable(f"{source}: download failed ({_reason(error)})") from error
        text = response.text
    else:
        text = Path(source).read_text(encoding="utf-8-sig")
    data = yaml.safe_load(text)
    if data is None:
        return {}
    if not isinstance(data, dict):
        raise ValueError(f"{source}: expected a YAML mapping")
    return data


def save_yaml(path: str | Path, data: dict) -> None:
    """把映射写成 YAML 文件。
    Write a mapping as a YAML file.
    """
    from xrobot.generate_main import atomic_write

    atomic_write(Path(path), yaml.safe_dump(data, sort_keys=False, allow_unicode=True))


def _relative(origin: str | Path, value: str) -> str:
    """把 origin 中写的相对地址解析为绝对地址；远程地址原样返回。
    Resolve an address written in origin relative to it; remote addresses are kept.
    """
    value = str(value)
    if value.startswith(("https://", "http://", "file://", "git@", "ssh://")):
        return value
    if str(origin).startswith(("https://", "http://")):
        return urljoin(str(origin), value)
    return str((Path(origin).resolve().parent / value).resolve())


def _scalar(value: str) -> str:
    """一个字符串写成 YAML 标量的文本，需要时加引号。
    The YAML scalar text of a string, quoted when needed.
    """
    return yaml.safe_dump([value], allow_unicode=True, width=1 << 20)[2:].rstrip("\n")


def _source_entries(path: str | Path) -> list:
    """sources.yaml 中的源条目。
    The Source entries of a sources.yaml.

    Raises:
        ValueError: sources 不是列表，或某一项没有 url。
            sources is not a list, or an entry has no url.
    """
    entries = load_yaml(path).get("sources") or []
    if not isinstance(entries, list):
        raise ValueError(f"{path}: sources must be a list")
    for entry in entries:
        if not isinstance(entry, dict) or not entry.get("url"):
            raise ValueError(f"{path}: every source needs a url")
    return entries


def _entry_repo(value: object) -> object:
    """index 条目的仓库地址：字符串条目本身，或映射条目的 repo。
    The repository of an index entry: a string entry itself, or the repo of a mapping.
    """
    return value.get("repo", value.get("source")) if isinstance(value, dict) else value


def _append_item(path: Path, key: str, item: list[str], new_file: str) -> None:
    """在顶层列表 key 的末尾追加一项；文件其余内容、注释和换行符保持不变。
    Append one item to the top-level list ``key``; the rest of the file, its comments and
    its line endings stay as they are.

    Args:
        path: 要编辑的文件。
            The file to edit.
        key: 顶层列表的键。
            The key of the top-level list.
        item: 新条目的各行，不含 "- " 和缩进。
            The lines of the new item, without "- " and indentation.
        new_file: 文件不存在时的初始内容。
            The initial content when the file does not exist.

    Raises:
        ValueError: key 不是列表，或是非空的流式列表。
            key is not a list, or is a non-empty flow list.
    """
    from xrobot.generate_main import atomic_write

    text = path.read_bytes().decode("utf-8-sig") if path.exists() else new_file
    newline = "\r\n" if "\r\n" in text else "\n"
    lines = text.replace("\r\n", "\n").split("\n")
    root = yaml.compose("\n".join(lines), Loader=yaml.BaseLoader)
    if root is not None and not isinstance(root, yaml.MappingNode):
        raise ValueError(f"{path}: expected a YAML mapping")
    pair = next((p for p in root.value if p[0].value == key), None) if root else None
    if pair is None:
        while lines and not lines[-1].strip():
            lines.pop()
        lines += [f"{key}:", "  - " + item[0]] + ["    " + line for line in item[1:]]
    else:
        name, node = pair
        if isinstance(node, yaml.SequenceNode) and node.value and not node.flow_style:
            last = node.value[-1]
            prefix = lines[last.start_mark.line][: last.start_mark.column]
            end = last.end_mark.line + (1 if last.end_mark.column else 0)
            at = max(end, last.start_mark.line + 1)
            lines[at:at] = [prefix + item[0]] + [" " * len(prefix) + line for line in item[1:]]
        elif (isinstance(node, yaml.SequenceNode) and not node.value) or (
            isinstance(node, yaml.ScalarNode) and node.value in ("", "~", "null")
        ):
            row, column = name.start_mark.line, name.start_mark.column
            lines[row] = lines[row][:column] + f"{key}:"
            indent = " " * (column + 2)
            lines[row + 1 : row + 1] = [indent + "- " + item[0]] + [
                indent + "  " + line for line in item[1:]
            ]
        else:
            raise ValueError(f"{path}: write {key} as a block list (one '- ' item per line) first")
    atomic_write(path, ("\n".join(lines).rstrip("\n") + "\n").replace("\n", newline))


class ModuleSource:
    """一个源：一个 index.yaml 及其在 sources.yaml 中的优先级。
    One Source: an index.yaml and its priority in sources.yaml.
    """

    def __init__(self, url: str, priority: int = 0) -> None:
        self.url, self.priority = str(url), int(priority)
        self.namespace = None
        self.mirror_of = None
        self.entries: dict[str, dict] = {}

    def load_index(self) -> None:
        """读取 index.yaml，得出每个条目的包 id 和记录。
        Read the index.yaml and derive the package id and record of every entry.

        Raises:
            ValueError: 条目不合法；信息以 index 地址和条目开头。
                An invalid entry; the message starts with the index and the entry.
        """
        data = load_yaml(self.url)
        self.namespace = data.get("namespace")
        self.mirror_of = data.get("mirror_of")
        self.entries = {}
        seen = set()
        for group, kind in (("modules", "module"), ("bsps", "bsp"), ("packages", None)):
            values = data.get(group) or []
            if not isinstance(values, list):
                raise ValueError(f"{self.url}: {group} must be a list")
            for value in values:
                record = self._record(value, kind)
                if record["id"].casefold() in seen:
                    raise ValueError(f"{self.url}: {record['id']} is listed more than once")
                seen.add(record["id"].casefold())
                self.entries[record["id"]] = record

    def _derive_id(self, repo: str) -> str | None:
        """没有写 id 的条目的包 id：镜像源用 mirror_of，GitHub 地址取 owner/repo，否则用 namespace。
        The id of an entry without one: mirror_of for a mirror, owner/repo of a GitHub URL,
        else the namespace of the index.
        """
        name = extract_name_from_url(repo)
        if self.mirror_of:
            return f"{self.mirror_of}/{name}"
        match = _GITHUB.search(repo)
        if match:
            return match.group(1)
        if self.namespace:
            return f"{self.namespace}/{name}"
        return None

    def _record(self, value: object, kind: str | None) -> dict:
        """一个条目的记录：包 id、类型、仓库地址，模块另有状态标签。
        The record of one entry: package id, type and repository, plus the status labels of
        a Module.

        Raises:
            ValueError: 条目不合法。
                The entry is invalid.
        """
        if not isinstance(value, (str, dict)):
            raise ValueError(f"{self.url}: {value!r}: an entry is a repository URL or a mapping")
        entry = {"repo": value} if isinstance(value, str) else dict(value)
        repo = _entry_repo(entry)
        where = f"{self.url}: {entry.get('id') or repo or value}"
        package_type = entry.get("type", kind)
        if package_type not in ("module", "bsp") or (kind and package_type != kind):
            raise ValueError(f"{where}: type must be {kind or 'module or bsp'}")
        if not isinstance(repo, str) or not repo:
            raise ValueError(f"{where}: missing repo URL")
        identity = entry.get("id") or self._derive_id(repo)
        if identity is None:
            raise ValueError(
                f"{where}: cannot derive owner/Repo; add `id: owner/Repo` to the entry or "
                "`namespace:` to the index"
            )
        try:
            validate_id(identity)
        except ValueError as error:
            raise ValueError(f"{where}: {error}") from None
        record = {"id": identity, "type": package_type, "repo": _relative(self.url, repo)}
        record["source"] = self.url
        if package_type == "bsp":
            # BSP 条目只用于发现仓库，不要求平台、MCU、构建或验证标签。
            # A BSP entry only makes the repository discoverable; no platform, MCU, build or
            # validation labels are required.
            return record
        where = f"{self.url}: {identity}"
        status = entry.get("status", "community")
        if status not in STATUSES:
            raise ValueError(f"{where}: unknown status {status}; use {', '.join(STATUSES)}")
        if status != "community" and not (entry.get("tested_ref") and entry.get("tested_libxr")):
            raise ValueError(f"{where}: status {status} needs tested_ref and tested_libxr")
        record["status"] = status
        for field, text in entry.items():
            if field not in ("id", "type", "repo", "source", "status"):
                record[field] = str(text) if field.startswith("tested_") else text
        return record


def _ordered(record: dict) -> dict:
    """按固定顺序排列记录的字段，其他字段排在后面。
    The record with its fields in a fixed order, other fields last.
    """
    first = {field: record[field] for field in FIELDS if field in record}
    return first | {field: value for field, value in record.items() if field not in first}


class SourceManager:
    """sources.yaml 列出的全部源合成的包表。
    The package table combined from every Source listed in sources.yaml.

    Attributes:
        packages: 每个包 id 生效的记录。repo 是 setup 拉取代码的地址（有镜像时为镜像），
            canonical 是写入 xrobot.lock 的原仓库地址。
            The effective record of each package id. repo is where setup fetches from (the
            mirror when there is one); canonical is the original repository that
            xrobot.lock records.
        all_module_candidates: 每个包 id 在各个源中的 (仓库地址, 源)，按优先级排列，含镜像。
            (repository, Source) of each package id in every Source by priority, mirrors
            included.
    """

    def __init__(self, sources_yaml: str | Path) -> None:
        self.sources: list[ModuleSource] = []
        self.packages: dict[str, dict] = {}
        self.all_module_candidates: dict[str, list] = {}
        self._keys: dict[str, str] = {}
        if str(sources_yaml).startswith(("https://", "http://")) or Path(sources_yaml).exists():
            self.load_sources(sources_yaml)

    def load_sources(self, path: str | Path) -> None:
        """读取 sources.yaml 及其列出的全部 index.yaml，建立包表。
        Read sources.yaml and every index.yaml it lists, and build the package table.

        priority 数值小的源生效。镜像源不参与这一选择，只提供拉取地址。
        The Source with the lower priority number wins. Mirror Sources take no part in
        that choice; they only supply fetch URLs.

        Raises:
            ValueError: 条目不合法，或同一优先级的两个非镜像源给出不同的仓库。
                An invalid entry, or two non-mirror Sources of the same priority list
                different repositories.
        """
        self.sources = []
        for entry in _source_entries(path):
            source = ModuleSource(_relative(path, entry["url"]), entry.get("priority", 0))
            source.load_index()
            self.sources.append(source)
        self.sources.sort(key=lambda source: source.priority)
        listed: dict[str, list] = {}
        self._keys = {}
        for source in self.sources:
            for identity, record in source.entries.items():
                key = self._keys.setdefault(identity.casefold(), identity)
                listed.setdefault(key, []).append((record, source))
        self.all_module_candidates = {
            key: [(record["repo"], source) for record, source in rows]
            for key, rows in listed.items()
        }
        self.packages = {}
        for key, rows in listed.items():
            originals = [row for row in rows if not row[1].mirror_of]
            chosen, source = (originals or rows)[0]
            for other, rival in originals[1:]:
                if rival.priority == source.priority and other["repo"] != chosen["repo"]:
                    raise ValueError(
                        f"{key}: {source.url} and {rival.url} have the same priority "
                        f"{source.priority} but list different repositories ({chosen['repo']}, "
                        f"{other['repo']}); give one of them another priority in sources.yaml"
                    )
            record = dict(chosen, canonical=chosen["repo"])
            record["repo"] = self.mirror_url(key) or chosen["repo"]
            self.packages[key] = _ordered(record)

    def resolve_id(self, name: str, kind: str | None = None) -> str:
        """按 owner/Repo 或 Repo 查找包 id，不区分大小写。
        Find a package id by owner/Repo or Repo, ignoring case.

        Raises:
            ValueError: 找不到，或只写 Repo 时有多个同名包。
                Not found, or several packages share the Repo name.
        """
        wanted = name.casefold()
        candidates = [
            identity
            for identity, record in self.packages.items()
            if (not kind or record["type"] == kind)
            and (identity if "/" in name else identity.rsplit("/", 1)[-1]).casefold() == wanted
        ]
        if not candidates:
            raise ValueError(f"Package not found: {name}")
        if len(candidates) != 1:
            raise ValueError(f"Ambiguous package {name}; specify {', '.join(sorted(candidates))}")
        return candidates[0]

    def find_module(self, identity: str) -> list:
        """包在各个源中的 (仓库地址, 源)，含镜像。
        (repository, Source) of the package in every Source, mirrors included.
        """
        return self.all_module_candidates[self.resolve_id(identity)]

    def mirror_url(self, identity: str) -> str | None:
        """镜像源为这个包给出的拉取地址；没有镜像时为 None。
        The fetch URL a mirror Source gives for the package; None without a mirror.
        """
        key = self._keys.get(identity.casefold())
        candidates = self.all_module_candidates.get(key, [])
        return next((repo for repo, source in candidates if source.mirror_of), None)


def add_source(sources_yaml: str | Path, url: str, priority: int = 0) -> bool:
    """向 sources.yaml 追加一个源；已列出的地址不重复添加。
    Append a Source to sources.yaml; a URL that is already listed is not added again.

    Returns:
        是否追加了。
        Whether the Source was appended.
    """
    path = Path(sources_yaml)
    if path.exists() and any(entry["url"] == url for entry in _source_entries(path)):
        return False
    _append_item(path, "sources", [f"url: {_scalar(url)}", f"priority: {int(priority)}"], "")
    return True


def create_sources_yaml(path: str | Path) -> None:
    """写入只含官方源的 sources.yaml。
    Write a sources.yaml that lists the official Source only.
    """
    from xrobot.generate_main import atomic_write

    atomic_write(Path(path), SOURCES_TEMPLATE)


def create_index_yaml(
    path: str | Path, namespace: str = "local", mirror_of: str | None = None
) -> None:
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


def add_index_entry(index_yaml: str | Path, repo_url: str) -> bool:
    """向 index.yaml 追加一个模块仓库；文件不存在时新建，已列出的仓库不重复添加。
    Append a Module repository to index.yaml; the file is created when missing, and a
    repository that is already listed is not added again.

    Returns:
        是否追加了。
        Whether the repository was appended.
    """
    path = Path(index_yaml)
    if path.exists():
        listed = [_entry_repo(value) for value in load_yaml(path).get("modules") or []]
        if repo_url in listed:
            return False
    _append_item(path, "modules", [_scalar(repo_url)], "namespace: local\n")
    return True
