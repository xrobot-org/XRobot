"""测试共用的 BSP、上游仓库、编译和命令行辅助。
Shared BSP, upstream repository, compiler and command-line fixtures for the tests.

BspTestCase 是一个临时 BSP：模块是就地提交的 git 检出，由指向这些提交的 xrobot.lock
锁定，与 xrobot setup 之后的状态相同。UpstreamTestCase 是列在本地 index.yaml 中的上游
模块仓库（master 和 dev 分支），以及一个 sources.yaml 指向它的空 BSP。CxxMixin 用
$CXX（默认 g++）编译生成的头文件，没有编译器时跳过。CliMixin 在进程内运行 xrobot 命令。
BspTestCase is a temporary BSP whose Modules are git checkouts committed in place and
pinned by an xrobot.lock pointing at those commits, as xrobot setup leaves them.
UpstreamTestCase provides upstream Module repositories (branches master and dev) in a
local index.yaml and an empty BSP whose sources.yaml points at it. CxxMixin compiles the
generated header with $CXX (default g++) and skips without it. CliMixin runs xrobot
commands in process.
"""

import contextlib
import io
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml

from xrobot import __version__
from xrobot.cli import main as cli_main
from xrobot.project import Project

GIT_OPTIONS = [
    "-c",
    "commit.gpgsign=false",
    "-c",
    "tag.gpgsign=false",
    "-c",
    "core.autocrlf=false",
    "-c",
    "init.defaultBranch=master",
    "-c",
    "advice.detachedHead=false",
]
GIT_ENV = {
    "GIT_AUTHOR_NAME": "Fixture",
    "GIT_AUTHOR_EMAIL": "fixture@example.invalid",
    "GIT_COMMITTER_NAME": "Fixture",
    "GIT_COMMITTER_EMAIL": "fixture@example.invalid",
    "GIT_TERMINAL_PROMPT": "0",
    "GIT_CONFIG_NOSYSTEM": "1",
}

# 测试断言英文输出；中文输出的测试自己设置 XR_LANG。
# Tests assert English output; tests of Chinese output set XR_LANG themselves.
os.environ["XR_LANG"] = "en"

CXX = os.environ.get("CXX", "g++")
HAVE_CXX = shutil.which(CXX) is not None

LIBXR_STUB = "#pragma once\n"
THREAD_STUB = (
    "#pragma once\n#include <cstdlib>\n"
    "namespace LibXR { struct Thread { static void Sleep(unsigned) { std::_Exit(0); } }; }\n"
)


def run_git(repo, *args, check=True):
    """以固定身份运行 git，不受用户和系统配置影响；返回去掉首尾空白的输出。
    Run git with a fixed identity and no user or system configuration; return the stripped output.
    """
    command = ["git"] + GIT_OPTIONS + (["-C", str(repo)] if repo is not None else []) + list(args)
    result = subprocess.run(
        command,
        capture_output=True,
        encoding="utf-8",
        errors="replace",
        env=dict(os.environ, **GIT_ENV),
        timeout=60,
    )
    if check and result.returncode:
        raise AssertionError(f"git {' '.join(args)} failed in {repo}:\n{result.stderr}")
    return result.stdout.strip()


EMPTY_MANIFEST = "/* === MODULE MANIFEST V2 ===\n=== END MANIFEST === */\n"


def manifest_block(description="fixture", depends=None, **extra):
    """模块头文件中的 MODULE MANIFEST V2 注释块。
    The MODULE MANIFEST V2 comment block of a Module header.
    """
    data = {"module_description": description}
    if depends is not None:
        data["depends"] = depends
    data.update(extra)
    return (
        "/* === MODULE MANIFEST V2 ===\n"
        + yaml.safe_dump(data, sort_keys=False)
        + "=== END MANIFEST === */\n"
    )


class TestCase(unittest.TestCase):
    """unittest.TestCase 加上逐字比较报错文本的断言。
    unittest.TestCase with an assertion that compares the error text exactly.
    """

    @contextlib.contextmanager
    def assertRaisesMessage(self, exception, message):
        """断言代码块抛出 exception，且报错文本与 message 相同。
        Assert that the block raises exception whose text equals message.
        """
        with self.assertRaises(exception) as context:
            yield context
        self.assertEqual(str(context.exception), message)


class TempDirTestCase(TestCase):
    """每个测试一个临时目录，以及按 UTF-8 读写文件的辅助。
    A temporary directory per test and helpers that read and write UTF-8 files.
    """

    def setUp(self):
        super().setUp()
        self._temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self._temporary.cleanup)
        self.tmp = Path(self._temporary.name).resolve()

    def write(self, path, text):
        """按 UTF-8 写文件（相对路径以 root 为基准），返回路径。
        Write a UTF-8 file (relative paths are under root) and return its path.
        """
        path = Path(path)
        if not path.is_absolute():
            path = self.root / path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode("utf-8"))
        return path

    def read(self, path):
        """按 UTF-8 读文件（相对路径以 root 为基准）。
        Read a UTF-8 file (relative paths are under root).
        """
        path = Path(path)
        if not path.is_absolute():
            path = self.root / path
        return path.read_bytes().decode("utf-8")


class BspTestCase(TempDirTestCase):
    """模块为 git 检出、由 xrobot.lock 锁定的临时 BSP。
    A temporary BSP whose Modules are git checkouts pinned by xrobot.lock.
    """

    owner = "team"

    def setUp(self):
        super().setUp()
        self.root = self.tmp / "bsp"
        self.locked = {}
        self.write("Modules/modules.yaml", f"xrobot: {__version__}\nmodules: []\n")
        (self.root / "User").mkdir(parents=True)
        self.write_lock()

    @property
    def project(self):
        """这个 BSP 的 Project。
        The Project of this BSP.
        """
        return Project(self.root)

    def module(self, name, body, owner=None, manifest=None, extra_headers=None):
        """写入 Modules/<owner>/<name>/<name>.hpp，提交并锁定到 xrobot.lock。
        Write Modules/<owner>/<name>/<name>.hpp, commit it and pin it in the lock.
        """
        owner = owner or self.owner
        identity = f"{owner}/{name}"
        folder = self.root / "Modules" / owner / name
        text = (
            "#pragma once\n" + (manifest if manifest is not None else EMPTY_MANIFEST) + body + "\n"
        )
        self.write(folder / (name + ".hpp"), text)
        for header, content in (extra_headers or {}).items():
            self.write(folder / header, content)
        if not (folder / ".git").exists():
            run_git(folder, "init", "-q")
        run_git(folder, "add", "-A")
        run_git(folder, "commit", "-q", "--allow-empty", "-m", "fixture")
        self.locked[identity] = run_git(folder, "rev-parse", "HEAD")
        self.write_lock()
        return folder

    def write_lock(self):
        """按已提交的模块写 xrobot.lock。
        Write xrobot.lock for the committed Modules.
        """
        data = {
            "version": 1,
            "requests": [{"id": i, "ref": None} for i in sorted(self.locked)],
            "modules": {
                i: {"repo": f"https://example.invalid/{i}.git", "commit": c}
                for i, c in sorted(self.locked.items())
            },
        }
        self.write("xrobot.lock", yaml.safe_dump(data, sort_keys=False))

    def entry(self, text, name="app_main.cpp"):
        """写入 User/ 下的入口源文件。
        Write an entry source under User/.
        """
        return self.write("User/" + name, text)

    def config(self, data, name="xrobot.yaml"):
        """写入 User/ 下的配置（文本或数据）。
        Write a configuration under User/ (text or data).
        """
        text = (
            data
            if isinstance(data, str)
            else yaml.safe_dump(data, sort_keys=False, allow_unicode=True)
        )
        return self.write("User/" + name, text)

    def generate(self, data=None, entry=None, name="xrobot.yaml"):
        """写入配置（和入口源文件），生成 User/xrobot_main.hpp 并返回其文本。
        Write the configuration (and entry) and generate User/xrobot_main.hpp; return its text.
        """
        from xrobot.generate_main import generate

        if entry is not None or not (self.root / "User/app_main.cpp").exists():
            self.entry(
                entry
                if entry is not None
                else '#include "xrobot_main.hpp"\nint main() { XROBOT_MAIN(); }\n'
            )
        path = self.config(data, name) if data is not None else self.root / "User" / name
        return generate(self.project, path)


class CxxMixin:
    """用生成的头文件编译（并运行）入口源文件。
    Compile (and run) the entry source against the generated header.
    """

    standard = os.environ.get("XR_CXX_STANDARD", "c++20")

    def setUp(self):
        super().setUp()
        self.stub("libxr.hpp", LIBXR_STUB)
        self.stub("thread.hpp", THREAD_STUB)

    def stub(self, name, text):
        """写入编译时使用的替身头文件（如 libxr.hpp）。
        Write a stand-in header used when compiling (such as libxr.hpp).
        """
        self.write(self.tmp / "stub" / name, text)

    def include_flags(self):
        """替身头文件、User/ 和每个模块目录的 -I 参数。
        The -I flags of the stand-in headers, User/ and every Module folder.
        """
        flags = ["-I" + str(self.tmp / "stub"), "-I" + str(self.root / "User")]
        modules = self.root / "Modules"
        for owner in sorted(p for p in modules.iterdir() if p.is_dir()):
            flags += ["-I" + str(p) for p in sorted(owner.iterdir()) if p.is_dir()]
        return flags

    def compile(self, source=None, expected=True, execute=True, extra=(), warnings=True):
        """编译入口源文件并按需运行；返回编译器或程序的输出。
        Compile the entry source and run it if asked; return the compiler or program output.
        """
        source = Path(source) if source else self.root / "User/app_main.cpp"
        output = self.tmp / ("program.exe" if execute else "object.o")
        command = [CXX, "-std=" + self.standard]
        if warnings:
            command += ["-Wall", "-Wextra", "-Werror"]
        command += ["-O1"] + self.include_flags() + list(extra)
        if not execute:
            command.append("-c")
        command += [str(source), "-o", str(output)]
        result = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            encoding="utf-8",
            errors="replace",
            timeout=120,
        )
        if not expected:
            self.assertNotEqual(result.returncode, 0, "compilation unexpectedly succeeded")
            return result.stdout
        self.assertEqual(result.returncode, 0, result.stdout)
        if execute:
            run = subprocess.run(
                [str(output)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                encoding="utf-8",
                errors="replace",
                timeout=30,
            )
            self.assertEqual(run.returncode, 0, run.stdout)
            return run.stdout
        return result.stdout


requires_cxx = unittest.skipUnless(HAVE_CXX, f"C++ compiler {CXX} not available on this host")


class UpstreamTestCase(TempDirTestCase):
    """列在本地 index 中的上游模块仓库，以及使用它的空 BSP。
    Upstream Module repositories in a local index, and an empty BSP using it.
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp / "bsp"
        self.modules = self.root / "Modules"
        self.modules.mkdir(parents=True)
        self.index = self.tmp / "index.yaml"
        self.entries = []
        self.write_yaml(self.modules / "sources.yaml", {"sources": [{"url": str(self.index)}]})
        self.write_yaml(self.index, {"packages": []})
        self.configure([])

    @property
    def project(self):
        """这个 BSP 的 Project。
        The Project of this BSP.
        """
        return Project(self.root)

    def write_yaml(self, path, value):
        """把数据写成 YAML 文件。
        Write data as a YAML file.
        """
        return self.write(path, yaml.safe_dump(value, sort_keys=False, allow_unicode=True))

    def upstream(self, identity, depends=None, kind="module", branches=("dev",), listed=True):
        """在 master（以及 branches）上创建上游仓库，并列入 index。
        Create an upstream repository on master (plus branches) and list it in the index.
        """
        path = self.tmp / "upstream" / identity
        path.mkdir(parents=True)
        run_git(path, "init", "-q", "-b", "master")
        self.commit(path, depends or [], "initial")
        for branch in branches:
            run_git(path, "branch", branch)
        if listed:
            self.entries.append({"id": identity, "repo": str(path), "type": kind})
            self.write_yaml(self.index, {"packages": self.entries})
        return path

    def commit(self, path, depends, message):
        """在仓库中提交一个依赖给定模块的头文件，返回 commit。
        Commit a header with the given dependencies to a repository and return the commit.
        """
        name = Path(path).name
        text = (
            "#pragma once\n"
            + manifest_block(message, depends)
            + f"class {name} {{ public: {name}() {{}} }};\n"
        )
        self.write(Path(path) / (name + ".hpp"), text)
        self.write(
            Path(path) / "CMakeLists.txt",
            'target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")\n',
        )
        run_git(path, "add", "-A")
        run_git(path, "commit", "-q", "-m", message)
        return run_git(path, "rev-parse", "HEAD")

    def configure(self, requests, pin=__version__):
        """写 modules.yaml：模块请求和 xrobot 版本锁定。
        Write modules.yaml: the Module requests and the xrobot pin.
        """
        data = {}
        if pin is not None:
            data["xrobot"] = pin
        data["modules"] = requests
        self.write_yaml(self.modules / "modules.yaml", data)

    def sync(self, cwd=None, **kwargs):
        """在 cwd（默认 BSP 根目录）运行 sync_modules，返回写入的锁。
        Run sync_modules from cwd (default: the BSP root) and return the written lock.
        """
        from xrobot.lock import sync_modules

        previous = os.getcwd()
        os.chdir(str(cwd or self.root))
        try:
            return sync_modules(self.project, **kwargs)
        finally:
            os.chdir(previous)

    def lock_bytes(self):
        """xrobot.lock 的原始字节。
        The raw bytes of xrobot.lock.
        """
        return (self.root / "xrobot.lock").read_bytes()

    def head(self, identity):
        """BSP 中一个模块检出的 HEAD。
        HEAD of a Module checkout in the BSP.
        """
        return run_git(self.modules / identity, "rev-parse", "HEAD")


class CliMixin:
    """在进程内运行 xrobot 命令，返回退出码和输出。
    Run xrobot commands in process and return the exit code and output.
    """

    def run_cli(self, *argv, cwd=None):
        """在 cwd 运行 xrobot 命令，返回退出码、标准输出和标准错误。
        Run an xrobot command from cwd; return the exit code, stdout and stderr.
        """
        out, err = io.StringIO(), io.StringIO()
        previous = os.getcwd()
        os.chdir(str(cwd or self.root))
        try:
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                try:
                    code = cli_main([str(a) for a in argv])
                except SystemExit as exit:
                    code = exit.code
        finally:
            os.chdir(previous)
        return code, out.getvalue(), err.getvalue()

    def ok(self, *argv, cwd=None):
        """运行命令并断言成功，返回输出。
        Run a command, assert that it succeeds and return its output.
        """
        code, out, err = self.run_cli(*argv, cwd=cwd)
        self.assertEqual(code, 0, out + err)
        return out, err

    def fails(self, *argv, cwd=None, pattern=None, message=None):
        """运行命令并断言退出码为 1，标准错误匹配 pattern 或等于 message 加换行。
        Run a command, assert exit code 1 and that stderr matches pattern or is message plus a
        newline.
        """
        code, out, err = self.run_cli(*argv, cwd=cwd)
        self.assertEqual(code, 1, out + err)
        if pattern:
            self.assertRegex(err, pattern)
        if message is not None:
            self.assertEqual(err, message + "\n")
        return err
