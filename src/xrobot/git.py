"""xrobot 调用 git 的唯一入口：不交互、有超时、出错时给出一行信息。
The one way xrobot runs git: non-interactive, with a timeout, and a one-line error.

每启动一次 git 进程约需 20 ms（Windows），而每个模块检出都要查询，所以最常见的查询先读
.git 中的文件，读不出结果时再调用 git。
Starting git takes about 20 ms (Windows) and every Module checkout is queried, so the most
common query reads the file in .git first and runs git only when that gives no answer.
"""

import os
import re
import subprocess
from pathlib import Path

GIT_TIMEOUT = 300
_COMMIT = re.compile(r"[0-9a-f]{40}")


def git(path: str | Path | None, *args: str, check: bool = True) -> str | None:
    """在 path 中运行 git，返回去掉首尾空白的输出。
    Run git in path and return its stripped output.

    Returns:
        成功时为标准输出；check=False 且失败时为 None。
        The standard output on success; None on failure with check=False.

    Raises:
        ValueError: git 失败（check=True 时）或超时。
            git failed (with check=True) or timed out.
    """
    env = dict(os.environ, GIT_TERMINAL_PROMPT="0", GIT_OPTIONAL_LOCKS="0")
    command = ["git"] + (["-C", str(path)] if path else []) + list(args)
    where = f" in {path}" if path else ""
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            encoding="utf-8",
            errors="replace",
            env=env,
            timeout=GIT_TIMEOUT,
        )
    except subprocess.TimeoutExpired as error:
        raise ValueError(
            f"Git did not finish within {GIT_TIMEOUT} s{where}: {' '.join(args)}"
        ) from error
    if check and result.returncode:
        raise ValueError(f"Git failed{where}: {' '.join(args)}\n{result.stderr.strip()}")
    return result.stdout.strip() if result.returncode == 0 else None


def head_commit(folder: str | Path) -> str | None:
    """检出的 HEAD commit；仓库还没有 commit 时为 None。
    The HEAD commit of a checkout; None while the repository has no commit.

    分离 HEAD（模块检出的常态）直接读 .git/HEAD；在分支上或 .git 是文件时调用 git。
    A detached HEAD, the usual state of a Module checkout, is read from .git/HEAD; on a
    branch, or when .git is a file, git is run.
    """
    head = Path(folder) / ".git" / "HEAD"
    if head.is_file():
        text = head.read_text(encoding="utf-8", errors="replace").strip()
        if _COMMIT.fullmatch(text):
            return text
    return git(folder, "rev-parse", "--verify", "--quiet", "HEAD", check=False) or None


def checkout_state(folder: str | Path) -> tuple[str | None, str | None, bool]:
    """一次 git 调用得到检出的 HEAD commit、所在分支和是否有未提交的修改。
    The HEAD commit, the branch and whether there are uncommitted changes, from one git run.

    Returns:
        (commit 或 None, 分支名或 None（分离 HEAD）, 有修改或未跟踪的文件)。
        (commit or None, branch name or None for a detached HEAD, whether there are
        changes or untracked files).
    """
    output = git(folder, "status", "--porcelain=v2", "--branch")
    commit, branch, dirty = None, None, False
    for line in output.splitlines():
        if line.startswith("# branch.oid "):
            value = line.split(" ", 2)[2]
            commit = value if _COMMIT.fullmatch(value) else None
        elif line.startswith("# branch.head "):
            value = line.split(" ", 2)[2]
            branch = None if value == "(detached)" else value
        elif not line.startswith("#"):
            dirty = True
    return commit, branch, dirty


def origin_url(folder: str | Path) -> str:
    """检出的 origin 地址：先读 .git/config，读不到时调用 git。
    The origin URL of a checkout: read from .git/config, or from git when that fails.

    读到的是克隆时写入的地址，不经过 url.<base>.insteadOf 改写。
    The URL is the one the clone wrote, without url.<base>.insteadOf rewriting.

    Raises:
        ValueError: 没有 origin。
            There is no origin.
    """
    config = Path(folder) / ".git" / "config"
    if config.is_file():
        section = None
        for line in config.read_text(encoding="utf-8", errors="replace").splitlines():
            stripped = line.strip()
            if stripped.startswith("["):
                section = stripped
            elif section == '[remote "origin"]':
                key, _, value = stripped.partition("=")
                value = value.strip()
                # 带引号、转义或注释的值交给 git 解析。
                # A value with quotes, escapes or a comment is left to git.
                if key.strip() == "url" and value and not any(c in value for c in '"\\;#'):
                    return value
    return git(folder, "remote", "get-url", "origin")
