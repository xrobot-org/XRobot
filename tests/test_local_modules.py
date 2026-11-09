"""本地模块：modules.yaml 中的 ``path:`` 请求。
Local Modules: the ``path:`` requests of modules.yaml.
"""

import contextlib
import io
from pathlib import Path

import yaml
from fixtures import (
    EMPTY_MANIFEST,
    BspTestCase,
    CliMixin,
    UpstreamTestCase,
    manifest_block,
    run_git,
)
from test_generate_main import LED, REGISTERED, led

from xrobot.config_edit import add_module, remove_module
from xrobot.lock import read_modules_yaml, request
from xrobot.module_parser import discover_modules, locked_modules


class LocalModuleCase(UpstreamTestCase):
    """写本地模块目录、捕获警告的辅助。
    Helpers that write local Module folders and capture warnings.
    """

    def local(self, folder, depends=None, cmake=False):
        """在 folder 中写一个本地模块（主头文件，可选 CMakeLists.txt），返回 folder。
        Write a local Module into folder (primary header, optional CMakeLists.txt) and
        return folder.
        """
        folder = Path(folder)
        name = folder.name
        self.write(
            folder / (name + ".hpp"),
            "#pragma once\n"
            + manifest_block("local", depends)
            + f"class {name} {{ public: {name}() {{}} }};\n",
        )
        if cmake:
            self.write(
                folder / "CMakeLists.txt",
                'target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")\n',
            )
        return folder

    def sync_warnings(self, **kwargs):
        """运行 sync，返回 (lock, 标准错误)。
        Run sync and return (lock, stderr).
        """
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            lock = self.sync(**kwargs)
        return lock, err.getvalue()

    def cmake(self):
        """生成的 Modules/CMakeLists.txt。
        The generated Modules/CMakeLists.txt.
        """
        return (self.modules / "CMakeLists.txt").read_text(encoding="utf-8")


class Requests(LocalModuleCase):
    """path 请求的写法。
    How path requests are written.
    """

    def test_a_path_request_names_local_and_its_folder(self):
        self.assertEqual(
            request({"path": "Modules\\local\\Chassis/"}),
            {"id": "local/Chassis", "ref": None, "path": "Modules/local/Chassis"},
        )
        self.assertEqual(request({"path": "D:/work/Imu"})["id"], "local/Imu")

    def test_a_path_request_takes_no_ref_or_other_keys(self):
        with self.assertRaisesRegex(ValueError, "takes no ref"):
            request({"path": "Modules/local/A", "ref": "master"})
        with self.assertRaisesRegex(ValueError, "local Module and takes no ref"):
            request("local/A@master")
        with self.assertRaisesRegex(ValueError, "does not end in a Module folder"):
            request({"path": ".."})
        with self.assertRaisesRegex(ValueError, "empty"):
            request({"path": " "})

    def test_modules_yaml_requests_local_modules_only_by_path(self):
        self.configure(["local/A"])
        with self.assertRaisesRegex(ValueError, "reserved for local Modules"):
            read_modules_yaml(self.modules / "modules.yaml")

    def test_two_paths_with_the_same_folder_name_are_rejected(self):
        self.configure([{"path": "a/Chassis"}, {"path": "b/Chassis"}])
        with self.assertRaisesRegex(ValueError, "a Module folder name must be unique"):
            read_modules_yaml(self.modules / "modules.yaml")

    def test_the_sources_cannot_use_the_local_owner(self):
        self.upstream("local/A")
        self.configure(["local/A"])
        with self.assertRaisesRegex(ValueError, "reserved for local Modules"):
            self.sync()


class Resolution(LocalModuleCase):
    """解析本地模块并写入 lock。
    Resolving local Modules into the lock.
    """

    def test_a_folder_inside_the_bsp_is_locked_without_a_commit(self):
        self.local(self.modules / "local/Chassis", cmake=True)
        self.configure([{"path": "Modules/local/Chassis"}])
        lock, err = self.sync_warnings()
        self.assertEqual(err, "")
        self.assertEqual(lock["modules"], {})
        self.assertEqual(lock["local"], {"local/Chassis": {"path": "Modules/local/Chassis"}})
        self.assertEqual(
            lock["requests"], [{"id": "local/Chassis", "path": "Modules/local/Chassis"}]
        )
        self.assertEqual(yaml.safe_load(self.lock_bytes()), lock)
        self.assertIn(
            'include("${CMAKE_CURRENT_LIST_DIR}/local/Chassis/CMakeLists.txt")', self.cmake()
        )
        modules = discover_modules(self.modules, self.root / "xrobot.lock")
        self.assertEqual(modules["local/Chassis"]["name"], "Chassis")
        self.assertEqual(
            modules["local/Chassis"]["header"],
            (self.modules / "local/Chassis/Chassis.hpp").resolve(),
        )
        before = self.lock_bytes()
        self.sync(frozen=True)
        self.assertEqual(self.lock_bytes(), before)

    def test_a_lock_without_local_modules_has_no_local_key(self):
        self.upstream("team/A")
        self.configure(["team/A@master"])
        self.assertNotIn("local", self.sync())
        self.assertNotIn(b"local", self.lock_bytes())

    def test_dependencies_of_a_local_module_follow_the_bsp_branch(self):
        b = self.upstream("team/B")
        dev = self.commit(b, [], "on dev")
        run_git(b, "branch", "-f", "dev", dev)
        run_git(b, "reset", "-q", "--hard", "HEAD~1")
        run_git(self.root, "init", "-q", "-b", "dev")
        self.local(self.modules / "local/Chassis", ["team/B@same-or-dev"])
        self.configure([{"path": "Modules/local/Chassis"}])
        lock = self.sync()
        self.assertEqual(lock["modules"]["team/B"]["commit"], dev)
        self.assertEqual(self.head("team/B"), dev)
        self.assertEqual(set(lock["local"]), {"local/Chassis"})

    def test_a_local_module_may_depend_on_another_declared_one(self):
        self.local(self.modules / "local/Base")
        self.local(self.modules / "local/Top", ["local/Base"])
        self.configure([{"path": "Modules/local/Top"}, {"path": "Modules/local/Base"}])
        self.assertEqual(set(self.sync()["local"]), {"local/Base", "local/Top"})
        self.sync(frozen=True)

    def test_a_local_dependency_must_be_declared(self):
        self.local(self.modules / "local/Top", ["local/Base"])
        self.configure([{"path": "Modules/local/Top"}])
        with self.assertRaisesRegex(ValueError, "does not declare; add `- path: <folder>`"):
            self.sync()

    def test_a_module_from_the_sources_cannot_depend_on_a_local_module(self):
        self.upstream("team/A", ["local/Base"])
        self.local(self.modules / "local/Base")
        self.configure(["team/A@master", {"path": "Modules/local/Base"}])
        with self.assertRaisesRegex(ValueError, "cannot depend on a local Module"):
            self.sync()

    def test_a_local_module_cannot_share_a_class_name_with_a_module_from_the_sources(self):
        self.upstream("team/A")
        self.local(self.modules / "local/A")
        self.configure(["team/A@master", {"path": "Modules/local/A"}])
        with self.assertRaisesRegex(ValueError, "both define the global class A"):
            self.sync()

    def test_a_missing_header_fails(self):
        (self.modules / "local/Chassis").mkdir(parents=True)
        self.configure([{"path": "Modules/local/Chassis"}])
        with self.assertRaisesRegex(ValueError, "has no .*Chassis.hpp"):
            self.sync()

    def test_a_folder_outside_the_bsp_warns_also_when_frozen(self):
        self.local(self.tmp / "shared/Gimbal", cmake=True)
        self.local(self.tmp / "abs/Imu")
        absolute = (self.tmp / "abs/Imu").resolve().as_posix()
        self.configure([{"path": "../shared/Gimbal"}, {"path": absolute}])
        lock, err = self.sync_warnings()
        self.assertIn("local/Gimbal is outside the BSP (../shared/Gimbal)", err)
        self.assertIn("local/Imu is outside the BSP", err)
        self.assertEqual(lock["local"]["local/Imu"], {"path": absolute})
        cmake = self.cmake()
        self.assertIn(
            'include("${CMAKE_CURRENT_LIST_DIR}/../../shared/Gimbal/CMakeLists.txt")', cmake
        )
        self.assertIn(f'target_include_directories(xr PUBLIC "{absolute}")', cmake)
        _, err = self.sync_warnings(frozen=True)
        self.assertIn("outside the BSP", err)

    def test_a_nested_repository_inside_the_bsp_warns(self):
        folder = self.local(self.modules / "local/Chassis")
        run_git(folder, "init", "-q")
        self.configure([{"path": "Modules/local/Chassis"}])
        _, err = self.sync_warnings()
        self.assertIn("has its own .git", err)

    def test_a_folder_the_bsp_ignores_fails_until_it_is_let_through(self):
        run_git(self.root, "init", "-q", "-b", "dev")
        self.write(self.root / ".gitignore", "/Modules/*/\n")
        self.local(self.modules / "local/Chassis")
        self.configure([{"path": "Modules/local/Chassis"}])
        with self.assertRaisesRegex(ValueError, "ignored by the BSP's .gitignore"):
            self.sync()
        self.write(self.root / ".gitignore", "/Modules/*/\n!/Modules/local/\n")
        self.assertIn("local/Chassis", self.sync()["local"])

    def test_frozen_fails_when_a_local_module_gains_a_dependency(self):
        self.upstream("team/B")
        folder = self.local(self.modules / "local/Chassis")
        self.configure([{"path": "Modules/local/Chassis"}])
        self.sync()
        self.local(folder, ["team/B@master"])
        with self.assertRaisesRegex(ValueError, "missing or ambiguous for team/B"):
            self.sync(frozen=True)
        self.assertIn("team/B", self.sync()["modules"])

    def test_frozen_fails_when_the_path_changes(self):
        self.local(self.modules / "local/Chassis")
        self.local(self.root / "Other/Chassis")
        self.configure([{"path": "Modules/local/Chassis"}])
        self.sync()
        self.configure([{"path": "Other/Chassis"}])
        with self.assertRaisesRegex(ValueError, "differs from xrobot.lock"):
            self.sync(frozen=True)
        self.assertEqual(self.sync()["local"], {"local/Chassis": {"path": "Other/Chassis"}})

    def test_leave_local_rejects_and_update_accepts_a_local_module(self):
        self.local(self.modules / "local/Chassis")
        self.configure([{"path": "Modules/local/Chassis"}])
        self.sync()
        with self.assertRaisesRegex(ValueError, "is a local Module; `--leave-local`"):
            self.sync(leave_local=["Chassis"])
        self.assertIn("local/Chassis", self.sync(update=["local/Chassis"])["local"])

    def test_locked_modules_reports_kind_and_path(self):
        self.upstream("team/A")
        self.local(self.tmp / "shared/Gimbal")
        self.configure(["team/A@master", {"path": "../shared/Gimbal"}])
        self.sync_warnings()
        states = {s["id"]: s for s in locked_modules(self.modules, self.root / "xrobot.lock")}
        self.assertEqual(states["team/A"]["kind"], "git")
        local = states["local/Gimbal"]
        self.assertEqual(
            (local["kind"], local["path"], local["in_tree"], local["commit"], local["status"]),
            ("local", "../shared/Gimbal", False, None, "ok"),
        )
        (self.tmp / "shared/Gimbal/Gimbal.hpp").unlink()
        with self.assertRaisesRegex(ValueError, "Local Module local/Gimbal .* has no"):
            discover_modules(self.modules, self.root / "xrobot.lock")


class Commands(CliMixin, LocalModuleCase):
    """xrobot module add/remove 处理本地模块。
    xrobot module add/remove with local Modules.
    """

    def test_module_add_path_and_remove_by_local_id(self):
        out, _ = self.ok("module", "add", "--path", "Modules/local/Chassis")
        self.assertIn("local/Chassis", out)
        requests, _ = read_modules_yaml(self.modules / "modules.yaml")
        self.assertEqual(requests[-1]["path"], "Modules/local/Chassis")
        self.fails("module", "add", "--path", "x/Chassis", pattern="already requested")
        self.ok("module", "remove", "local/Chassis")
        self.assertEqual(read_modules_yaml(self.modules / "modules.yaml")[0], [])

    def test_module_add_takes_a_request_or_a_path(self):
        self.fails("module", "add", pattern="either owner/Repo")
        self.fails("module", "add", "team/A", "--path", "x/A", pattern="either owner/Repo")
        self.fails("module", "add", "local/A", pattern="reserved for local Modules")

    def test_add_module_quotes_a_path_when_yaml_needs_it(self):
        path = self.modules / "modules.yaml"
        self.assertEqual(add_module(path, local_path="a: b/Chassis"), "local/Chassis")
        self.assertEqual(read_modules_yaml(path)[0][-1]["path"], "a: b/Chassis")
        remove_module(path, "local/Chassis")


class Generation(BspTestCase):
    """xrobot gen 使用本地模块。
    xrobot gen with local Modules.
    """

    def lock_local(self, path):
        """在 path 写入本地模块 Led，并把它写进 lock 的 local。
        Write the local Module Led at path and add it to the local key of the lock.
        """
        folder = (self.root / path).resolve()
        self.write(folder / "Led.hpp", "#pragma once\n" + EMPTY_MANIFEST + LED + "\n")
        data = yaml.safe_load((self.root / "xrobot.lock").read_text(encoding="utf-8"))
        data["requests"].append({"id": "local/Led", "path": path})
        data["local"] = {"local/Led": {"path": path}}
        self.write("xrobot.lock", yaml.safe_dump(data, sort_keys=False))
        self.entry(REGISTERED)

    def test_an_instance_of_a_local_module_inside_the_bsp(self):
        self.lock_local("Modules/local/Led")
        code = self.generate({"modules": [led()]})
        self.assertIn("// led: local/Led (User/xrobot.yaml:", code)
        self.assertIn('// xrobot: depends "../Modules/local/Led/Led.hpp"', code)

    def test_an_instance_of_a_local_module_outside_the_bsp(self):
        self.lock_local("../shared/Led")
        code = self.generate({"modules": [led()]})
        self.assertIn('// xrobot: depends "../../shared/Led/Led.hpp"', code)
