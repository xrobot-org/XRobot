"""核对发布记录与候选仓库是否一致；不构建、不打 tag、不发布。
Check a core release record against the candidate repositories; never build, tag or
publish.

LibXR、XRobot 和 LibXR_CppCodeGenerator 各自有版本号。记录写明每个候选的 commit、两个 Python
包的版本、两者都锁定的 xr-syntax 版本，以及各项验收结果：
LibXR, XRobot and LibXR_CppCodeGenerator are versioned independently. The record names
each candidate's exact commit, the version of each Python package, the xr-syntax version
both packages pin, and the acceptance gates:

    {
      "xr-syntax": "0.2.0",
      "repositories": {
        "libxr":   {"commit": "<40-hex>"},
        "xrobot":  {"commit": "<40-hex>", "version": "1.0.0"},
        "codegen": {"commit": "<40-hex>", "version": "6.0.0"}
      },
      "checks": {"automatic": "pass", "backends": "pass", "packages": "pass",
                 "modules": "pass", "docs": "pass", "bsp": "pass"}
    }
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from collections.abc import Sequence
from pathlib import Path

GATES = ("automatic", "backends", "packages", "modules", "docs", "bsp")
REPOSITORIES = ("libxr", "xrobot", "codegen")
PACKAGES = ("xrobot", "codegen")
VERSION = re.compile(r"\d+\.\d+\.\d+(?:(?:a|b|rc)\d+)?")
COMMIT = re.compile(r"[0-9a-f]{40}")


def project_table(repo: Path) -> str:
    """repo/pyproject.toml 中 [project] 表的文本。
    The text of the [project] table of repo/pyproject.toml.

    Raises:
        ValueError: 没有 [project] 表。
            There is no [project] table.
    """
    text = (Path(repo) / "pyproject.toml").read_text(encoding="utf-8-sig")
    match = re.search(r"^\[project\][ \t]*$(.*?)(?=^\[|\Z)", text, re.M | re.S)
    if not match:
        raise ValueError(f"Missing [project] in {Path(repo) / 'pyproject.toml'}")
    return match.group(1)


def package_version(repo: Path) -> str:
    """pyproject.toml 中写明的包版本。
    The package version written in pyproject.toml.

    Raises:
        ValueError: 没有写明版本。
            The version is not written out.
    """
    match = re.search(r"""^version\s*=\s*["']([^"']+)["']\s*$""", project_table(repo), re.M)
    if not match:
        raise ValueError(f"Expected an explicit package version in {repo}")
    return match.group(1)


def package_dependencies(repo: Path) -> list[str]:
    """pyproject.toml 中 dependencies 列出的依赖。
    The requirements listed in the dependencies of pyproject.toml.
    """
    match = re.search(r"^dependencies\s*=\s*\[(.*?)\]", project_table(repo), re.M | re.S)
    if not match:
        return []
    return re.findall(r"""["']([^"']+)["']""", match.group(1))


def normalized_name(requirement: str) -> str:
    """依赖的包名，按 PEP 503 规范化。
    The package name of a requirement, normalized as PEP 503 does.
    """
    name = re.match(r"\s*([A-Za-z0-9][A-Za-z0-9._-]*)", requirement)
    return re.sub(r"[-_.]+", "-", name.group(1)).lower() if name else ""


def xr_syntax_pin(repo: Path) -> tuple[str | None, str]:
    """repo 锁定的 xr-syntax 版本。
    The xr-syntax version repo pins.

    Returns:
        (版本, 依赖原文)；没有恰好锁定一个版本时版本为 None，第二项说明原因。
        (version, requirement text); the version is None when there is not exactly one
        exact pin, and the second item says why.
    """
    requirements = [r for r in package_dependencies(repo) if normalized_name(r) == "xr-syntax"]
    if len(requirements) != 1:
        return (
            None,
            "no xr-syntax dependency" if not requirements else "several xr-syntax dependencies",
        )
    requirement = requirements[0]
    match = re.fullmatch(rf"\s*xr[-_.]syntax\s*==\s*({VERSION.pattern})\s*", requirement, re.I)
    if not match:
        return None, requirement
    return match.group(1), requirement


def git(repo: Path, *args: str) -> str:
    """在 repo 中运行 git 并返回输出。
    Run git in repo and return its output.

    Raises:
        ValueError: git 失败，信息是 git 的报错。
            git failed; the message is git's error.
    """
    output = subprocess.run(
        ["git", "-C", str(repo), *args], capture_output=True, encoding="utf-8", timeout=20
    )
    if output.returncode:
        raise ValueError(output.stderr.strip())
    return output.stdout.strip()


def _check_repository(name: str, path: Path, row: dict) -> list[str]:
    """一个候选仓库与记录中对应一行的差异。
    The mismatches between one candidate repository and its row of the record.

    Raises:
        ValueError: git 或 pyproject.toml 读取失败。
            git or reading pyproject.toml failed.
    """
    failures = []
    commit = row.get("commit")
    if not isinstance(commit, str) or not COMMIT.fullmatch(commit):
        failures.append(f"{name}: the record needs the full 40-hex commit")
    if git(path, "status", "--porcelain"):
        failures.append(f"{name} is not a clean committed candidate")
    head = git(path, "rev-parse", "HEAD")
    if commit != head:
        failures.append(f"{name}: recorded commit {str(commit)[:12]} is not HEAD {head[:12]}")
    if name in PACKAGES:
        version = row.get("version")
        if not isinstance(version, str) or not VERSION.fullmatch(version):
            failures.append(f"{name}: the record needs a release version such as 1.0.0 or 1.0.0rc1")
        actual = package_version(path)
        if actual != version:
            failures.append(
                f"{name}: pyproject.toml version {actual} differs from the recorded {version}"
            )
    return failures


def check(repositories: dict[str, Path], record: dict) -> list[str]:
    """候选仓库与记录的全部差异；一个仓库读取失败时继续检查其余仓库。
    Every mismatch between the candidate repositories and the record; a repository that
    cannot be read does not stop the others from being checked.
    """
    failures = []
    rows = record.get("repositories") or {}
    readable = set()
    for name in REPOSITORIES:
        if name not in repositories:
            failures.append(f"{name} repository path is missing")
            continue
        try:
            failures += _check_repository(name, repositories[name], rows.get(name) or {})
            readable.add(name)
        except (OSError, ValueError) as error:
            failures.append(f"{name} ({repositories[name]}): {error}")
    pinned = record.get("xr-syntax")
    if not isinstance(pinned, str) or not VERSION.fullmatch(pinned):
        failures.append("the record needs the pinned xr-syntax version")
    for name in PACKAGES:
        if name not in readable:
            continue
        version, requirement = xr_syntax_pin(repositories[name])
        if version is None:
            failures.append(
                f"{name} must pin xr-syntax exactly (xr-syntax==X); found {requirement}"
            )
        elif version != pinned:
            failures.append(f"{name} pins xr-syntax=={version}, the record says {pinned}")
    for gate in GATES:
        if (record.get("checks") or {}).get(gate) != "pass":
            failures.append("Candidate acceptance is missing: " + gate)
    return failures


def main(argv: Sequence[str] | None = None) -> None:
    """命令行入口：记录与候选一致时退出码为 0，否则逐行列出差异并以 1 退出。
    Command-line entry: exit 0 when the record and the candidates agree, otherwise list
    the mismatches one per line and exit 1.
    """
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--libxr", required=True)
    parser.add_argument("--xrobot", required=True)
    parser.add_argument("--codegen", required=True)
    parser.add_argument("--record", required=True)
    args = parser.parse_args(argv)
    try:
        record = json.loads(Path(args.record).read_text(encoding="utf-8-sig"))
        failures = check({name: Path(getattr(args, name)) for name in REPOSITORIES}, record)
        if failures:
            parser.exit(1, "\n".join(failures) + "\n")
        print(
            "Release candidates and the acceptance record agree; no tag or publication performed."
        )
    except (OSError, ValueError, subprocess.TimeoutExpired) as error:
        parser.exit(1, str(error) + "\n")


if __name__ == "__main__":
    main()
