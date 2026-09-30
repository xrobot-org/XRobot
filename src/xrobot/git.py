"""xrobot 调用 git 的唯一入口：不交互、有超时、出错时给出一行信息。
The one way xrobot runs git: non-interactive, with a timeout, and a one-line error.
"""

import os
import subprocess
from pathlib import Path

GIT_TIMEOUT = 300


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
