"""本地模块：modules.yaml 中的 ``path:`` 请求。
Local Modules: the ``path:`` requests of modules.yaml.
"""

import contextlib
import io
import os
import subprocess
from pathlib import Path
from unittest import mock

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
from xrobot.describe import describe
from xrobot.lock import read_modules_yaml, request
from xrobot.module_parser import discover_modules, locked_modules
from xrobot.project import read_header_inputs


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
            request({"path": "Modules\\local\\Chassis/"}, allow_path=True),
            {"id": "local/Chassis", "ref": None, "path": "Modules/local/Chassis"},
        )
        self.assertEqual(request({"path": "D:/work/Imu"}, allow_path=True)["id"], "local/Imu")

    def test_a_path_request_takes_no_ref_or_other_keys(self):
        for value in (
            {"path": "Modules/local/A", "ref": "master"},
            {"id": "local/A", "path": "Modules/local/A"},
        ):
            with self.assertRaisesRegex(ValueError, "takes no ref"):
                request(value, allow_path=True)
        with self.assertRaisesRegex(ValueError, "local Module and takes no ref"):
            request("local/A@master")
        with self.assertRaisesRegex(ValueError, "does not end in a Module folder"):
            request({"path": ".."}, allow_path=True)
        with self.assertRaisesRegex(ValueError, "empty"):
            request({"path": " "}, allow_path=True)

    def test_a_path_cannot_carry_a_semicolon_or_a_control_character(self):
        for path in ("a;b/Chassis", "a\nb/Chassis", "a\tb/Chassis"):
            with self.assertRaisesRegex(ValueError, "semicolon or a control character"):
                request({"path": path}, allow_path=True)

    def test_only_modules_yaml_writes_a_path(self):
        with self.assertRaisesRegex(ValueError, "only Modules/modules.yaml writes its path"):
            request({"path": "Modules/local/A"}, canonical=True)
        self.local(self.modules / "local/Base")
        self.local(self.modules / "local/Top", [{"path": "Modules/local/Base"}])
        self.configure([{"path": "Modules/local/Top"}, {"path": "Modules/local/Base"}])
        with self.assertRaisesRegex(ValueError, "only Modules/modules.yaml writes its path"):
            self.sync()

    def test_modules_yaml_requests_local_modules_only_by_path(self):
        for value in ("local/A", "Local/A@master"):
            self.configure([value])
            with self.assertRaisesRegex(ValueError, "reserved for local Modules|takes no ref"):
                read_modules_yaml(self.modules / "modules.yaml")
        self.configure(["Local/A"])
        with self.assertRaisesRegex(ValueError, "Local/A uses the owner local"):
            read_modules_yaml(self.modules / "modules.yaml")

    def test_two_paths_with_the_same_folder_name_are_rejected(self):
        self.configure([{"path": "a/Chassis"}, {"path": "b/Chassis"}])
        with self.assertRaisesRegex(ValueError, "a Module folder name must be unique"):
            read_modules_yaml(self.modules / "modules.yaml")

    def test_the_sources_cannot_use_the_local_owner(self):
        # 只请求 team/B：加载 Source 的 index 时就报错，不是因为请求了 local/A。
        # Only team/B is requested: loading the index of the Source fails, not a request of
        # local/A.
        self.upstream("Local/A")
        self.upstream("team/B")
        self.configure(["team/B@master"])
        with self.assertRaisesRegex(ValueError, "Local/A uses the owner local, which is reserved"):
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

    def test_a_path_with_the_wrong_case_is_rejected_on_a_case_insensitive_file_system(self):
        self.local(self.modules / "local/ChassisIO")
        if not (self.modules / "local/chassisio").exists():
            self.skipTest("the file system is case-sensitive")
        self.configure([{"path": "Modules/local/chassisio"}])
        with self.assertRaisesRegex(ValueError, "names chassisio, but the folder is ChassisIO"):
            self.sync()
        with self.assertRaisesRegex(ValueError, "names chassisio, but the folder is ChassisIO"):
            add_module(self.modules / "modules.yaml", local_path="Modules/local/chassisio")

    def test_a_symlink_to_a_folder_with_another_name_is_rejected(self):
        real = self.local(self.tmp / "real/Bar")
        link = self.modules / "local/Foo"
        link.parent.mkdir(parents=True, exist_ok=True)
        try:
            os.symlink(str(real), str(link), target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks cannot be created here")
        self.configure([{"path": "Modules/local/Foo"}])
        with self.assertRaisesRegex(ValueError, "names Foo, but the folder is Bar"):
            self.sync()
        with self.assertRaisesRegex(ValueError, "names Foo, but the folder is Bar"):
            add_module(self.modules / "modules.yaml", local_path="Modules/local/Foo")

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
        with self.assertRaisesRegex(
            ValueError,
            r"ignored by the BSP's .gitignore, so Modules/local/Chassis/Chassis.hpp would not "
            r"be committed; add `!/Modules/local/` to .gitignore after the rule that ignores "
            r"it \(.gitignore:1:/Modules/\*/\)",
        ):
            self.sync()
        # 照提示加的一行就够了。
        # The one line the hint names is enough.
        self.write(self.root / ".gitignore", "/Modules/*/\n!/Modules/local/\n")
        self.assertIn("local/Chassis", self.sync()["local"])

    def test_an_ignored_source_file_fails_like_an_ignored_header(self):
        run_git(self.root, "init", "-q", "-b", "dev")
        self.write(self.root / ".gitignore", "*.cpp\n*.log\n")
        folder = self.local(self.modules / "local/Chassis")
        self.write(folder / "src/Chassis.cpp", "// source\n")
        self.write(folder / "build.log", "not built from\n")
        self.configure([{"path": "Modules/local/Chassis"}])
        with self.assertRaisesRegex(
            ValueError,
            r"so Modules/local/Chassis/src/Chassis.cpp would not be committed; add "
            r"`!/Modules/local/Chassis/src/Chassis.cpp`",
        ):
            self.sync()
        self.write(self.root / ".gitignore", "*.log\n")
        self.assertIn("local/Chassis", self.sync()["local"])

    def git_calls(self, **options):
        """运行 sync，返回它启动的所有 git 命令（去掉 -C 目录后的参数）。
        Run sync and return every git command it starts (the arguments after -C <folder>).
        """
        calls = []
        real = subprocess.run

        def counting(command, *args, **kwargs):
            """记录 git 命令，其余照常运行。
            Record git commands and run everything as usual.
            """
            if isinstance(command, list) and command[:1] == ["git"]:
                rest = command[1:]
                rest = rest[2:] if rest[:1] == ["-C"] else rest
                calls.append(rest + ([kwargs["input"]] if kwargs.get("input") else []))
            return real(command, *args, **kwargs)

        with mock.patch("subprocess.run", counting):
            self.sync_warnings(**options)
        return calls

    def test_the_gitignore_check_costs_the_same_for_any_number_of_local_modules(self):
        run_git(self.root, "init", "-q", "-b", "dev")
        self.write(self.root / ".gitignore", "/Modules/*/\n!/Modules/local/\n")
        names = ["A", "B", "C", "D"]
        for name in names:
            self.local(self.modules / "local" / name)
        counts, probes = [], []
        for count in (1, 4):
            self.configure([{"path": f"Modules/local/{name}"} for name in names[:count]])
            for options in ({}, {"frozen": True}):
                calls = self.git_calls(**options)
                counts.append(len([c for c in calls if "check-ignore" in c]))
                probes.append(len([c for c in calls if "--is-inside-work-tree" in c]))
        self.assertEqual(counts, [1, 1, 1, 1])
        # 其他代码对 rev-parse 的调用与本地模块的个数无关。
        # Calls of rev-parse by other code do not depend on the number of local Modules.
        self.assertEqual(probes[:2], probes[2:])

    def test_the_error_names_the_one_ignored_local_module(self):
        run_git(self.root, "init", "-q", "-b", "dev")
        self.write(self.root / ".gitignore", "/Modules/*/\n!/Modules/local/\n/Modules/local/B/\n")
        for name in "ABC":
            self.local(self.modules / "local" / name)
        self.configure([{"path": f"Modules/local/{name}"} for name in "ABC"])
        with self.assertRaisesRegex(
            ValueError, r"Local Module local/B \(Modules/local/B\) is ignored"
        ):
            self.sync()
        self.write(self.root / ".gitignore", "/Modules/*/\n!/Modules/local/\n")
        self.assertEqual(set(self.sync()["local"]), {"local/A", "local/B", "local/C"})

    def test_a_folder_that_is_not_checked_is_not_asked_about(self):
        run_git(self.root, "init", "-q", "-b", "dev")
        self.write(self.root / ".gitignore", "/Modules/*/\n")
        self.local(self.modules / "local/Nested")
        run_git(self.modules / "local/Nested", "init", "-q")
        self.local(self.tmp / "shared/Outside")
        self.configure([{"path": "Modules/local/Nested"}, {"path": "../shared/Outside"}])
        calls = self.git_calls()
        self.assertEqual([c for c in calls if "check-ignore" in c], [])
        # 另有一个要检查的本地模块时，git 只被问到它的文件。
        # With one more local Module that is checked, git is asked about its files only.
        self.write(self.root / ".gitignore", "/Modules/*/\n!/Modules/local/\n")
        self.local(self.modules / "local/Checked")
        self.configure(
            [
                {"path": "Modules/local/Nested"},
                {"path": "../shared/Outside"},
                {"path": "Modules/local/Checked"},
            ]
        )
        (asked,) = [c for c in self.git_calls() if "check-ignore" in c]
        self.assertEqual(asked[-1], "Modules/local/Checked/Checked.hpp\0")

    def test_a_bsp_outside_git_accepts_local_modules(self):
        self.write(self.root / ".gitignore", "/Modules/*/\n")
        self.local(self.modules / "local/Chassis")
        self.configure([{"path": "Modules/local/Chassis"}])
        self.assertIn("local/Chassis", self.sync()["local"])
        self.sync(frozen=True)

    def test_frozen_fails_when_a_local_module_gains_a_dependency(self):
        self.upstream("team/B")
        folder = self.local(self.modules / "local/Chassis")
        self.configure([{"path": "Modules/local/Chassis"}])
        self.sync()
        self.local(folder, ["team/B@master"])
        with self.assertRaisesRegex(ValueError, "missing or ambiguous for team/B"):
            self.sync(frozen=True)
        self.assertIn("team/B", self.sync()["modules"])

    def test_a_plain_setup_keeps_the_locked_commits_when_a_local_dependency_changes(self):
        self.upstream("team/A")
        self.upstream("team/B")
        folder = self.local(self.modules / "local/Chassis")
        self.configure(["team/A@master", {"path": "Modules/local/Chassis"}])
        locked = self.sync()["modules"]["team/A"]["commit"]
        self.local(folder, ["team/B@master"])
        lock = self.sync()
        self.assertEqual(lock["modules"]["team/A"]["commit"], locked)
        self.assertIn("team/B", lock["modules"])
        self.assertEqual(self.head("team/A"), locked)
        self.local(folder, [])
        lock = self.sync()
        self.assertEqual(set(lock["modules"]), {"team/A"})
        self.assertEqual(lock["modules"]["team/A"]["commit"], locked)

    def test_a_plain_setup_fails_on_a_lock_that_lacks_a_remote_root(self):
        self.upstream("team/A")
        self.local(self.modules / "local/Chassis")
        self.configure(["team/A@master", {"path": "Modules/local/Chassis"}])
        data = self.sync()
        del data["modules"]["team/A"]
        self.write("xrobot.lock", yaml.safe_dump(data, sort_keys=False))
        before = self.lock_bytes()
        for options in ({}, {"frozen": True}):
            with self.assertRaisesRegex(ValueError, "missing or ambiguous for team/A"):
                self.sync(**options)
            self.assertEqual(self.lock_bytes(), before)

    def test_a_plain_setup_fails_on_a_wrong_ref_of_a_remote_root(self):
        self.upstream("team/A")
        self.local(self.modules / "local/Chassis")
        self.configure(["team/A@master", {"path": "Modules/local/Chassis"}])
        data = self.sync()
        data["modules"]["team/A"]["resolved_ref"] = "other"
        self.write("xrobot.lock", yaml.safe_dump(data, sort_keys=False))
        with self.assertRaisesRegex(ValueError, "Locked ref does not match"):
            self.sync()

    def test_frozen_fails_when_a_local_module_drops_a_dependency(self):
        self.upstream("team/B")
        folder = self.local(self.modules / "local/Chassis", ["team/B@master"])
        self.configure([{"path": "Modules/local/Chassis"}])
        self.sync()
        self.local(folder, [])
        with self.assertRaisesRegex(ValueError, "outside the declared dependency closure"):
            self.sync(frozen=True)
        self.assertNotIn("team/B", self.sync()["modules"])

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

    def test_update_of_a_local_module_resolves_its_dependencies_again(self):
        self.upstream("team/A")
        self.upstream("team/B")
        folder = self.local(self.modules / "local/Chassis")
        self.configure(["team/A@master", {"path": "Modules/local/Chassis"}])
        before = self.sync()
        self.local(folder, ["team/B@master"])
        lock = self.sync(update=["local/Chassis"])
        self.assertEqual(set(lock["modules"]), {"team/A", "team/B"})
        self.assertEqual(lock["modules"]["team/A"], before["modules"]["team/A"])

    def test_a_dependency_cycle_between_local_modules_is_rejected(self):
        self.local(self.modules / "local/A", ["local/B"])
        self.local(self.modules / "local/B", ["local/A"])
        self.configure([{"path": "Modules/local/A"}, {"path": "Modules/local/B"}])
        with self.assertRaisesRegex(ValueError, "Package dependency cycle: local/A -> local/B"):
            self.sync()

    def test_a_dependency_cycle_made_after_locking_is_rejected_by_frozen(self):
        self.local(self.modules / "local/A")
        self.local(self.modules / "local/B", ["local/A"])
        self.configure([{"path": "Modules/local/A"}, {"path": "Modules/local/B"}])
        self.sync()
        self.local(self.modules / "local/A", ["local/B"])
        for options in ({"frozen": True}, {}):
            with self.assertRaisesRegex(ValueError, "Dependency cycle in project lock: local/A"):
                self.sync(**options)

    def test_release_ref_checks_the_remote_modules_and_not_the_local_ones(self):
        a = self.upstream("team/A")
        self.local(self.tmp / "shared/Gimbal")
        self.configure(["team/A@master", {"path": "../shared/Gimbal"}])
        _, err = self.sync_warnings(release_ref="refs/heads/dev")
        self.assertEqual(err.count("outside the BSP"), 1)
        run_git(a, "checkout", "-q", "-b", "feature/x")
        feature = self.commit(a, [], "feature work")
        run_git(a, "checkout", "-q", "master")
        self.configure(["team/A@feature/x", {"path": "../shared/Gimbal"}])
        self.assertEqual(self.sync_warnings()[0]["modules"]["team/A"]["commit"], feature)
        with self.assertRaisesRegex(ValueError, "is not on dev"):
            self.sync_warnings(frozen=True, release_ref="refs/heads/dev")

    def test_the_local_section_of_the_lock_follows_modules_yaml(self):
        # lock 的 local 只是 path 请求的副本：--frozen 拒绝不一致的副本，普通 setup 重写它，
        # 使用的目录始终是 modules.yaml 写的。
        # The local key of the lock only copies the path requests: --frozen refuses a copy
        # that differs, a plain setup rewrites it, and the folder used is always the one
        # modules.yaml names.
        self.local(self.modules / "local/Chassis", cmake=True)
        self.local(self.root / "Other/Chassis", cmake=True)
        self.configure([{"path": "Modules/local/Chassis"}])
        good = self.sync()
        expected = self.lock_bytes()
        for label, local in (
            ("missing", None),
            ("another folder", {"local/Chassis": {"path": "Other/Chassis"}}),
            ("an extra entry", {**good["local"], "local/Other": {"path": "Other/Chassis"}}),
            ("not a mapping", ["local/Chassis"]),
            ("entry without path", {"local/Chassis": {}}),
            ("entry that is not a mapping", {"local/Chassis": "Modules/local/Chassis"}),
            ("id of a remote Module", {"team/Chassis": {"path": "Modules/local/Chassis"}}),
        ):
            with self.subTest(label):
                data = {k: v for k, v in good.items() if k != "local"}
                if local is not None:
                    data["local"] = local
                self.write("xrobot.lock", yaml.safe_dump(data, sort_keys=False))
                with self.assertRaisesRegex(
                    ValueError, "local Modules in xrobot.lock differ from the `path:` requests"
                ):
                    self.sync(frozen=True)
                self.assertEqual(self.sync(), good)
                self.assertEqual(self.lock_bytes(), expected)
                self.assertIn("/local/Chassis/CMakeLists.txt", self.cmake())
                self.assertNotIn("Other", self.cmake())

    def test_locked_modules_reports_a_malformed_local_entry(self):
        self.local(self.modules / "local/Chassis")
        self.configure([{"path": "Modules/local/Chassis"}])
        good = self.sync()
        lock_path = self.root / "xrobot.lock"
        for local, reason in (
            ({"local/Chassis": {}}, "no valid path for local/Chassis"),
            ({"local/Chassis": {"path": 5}}, "no valid path for local/Chassis"),
            ({"local/Other": {"path": "Modules/local/Chassis"}}, "no valid path for local/Other"),
            ({"local/Modules": {"path": "Modules"}}, "is the BSP root, Modules/"),
        ):
            with self.subTest(local):
                self.write("xrobot.lock", yaml.safe_dump({**good, "local": local}))
                (state,) = locked_modules(self.modules, lock_path)
                self.assertEqual(state["status"], "broken")
                self.assertRegex(state["reason"], reason)
                with self.assertRaisesRegex(ValueError, reason):
                    discover_modules(self.modules, lock_path)
        self.write("xrobot.lock", yaml.safe_dump({**good, "local": [1]}))
        with self.assertRaisesRegex(ValueError, "invalid local Modules"):
            locked_modules(self.modules, lock_path)

    def test_a_local_module_does_not_change_how_remote_requests_resolve(self):
        # BSP 在 master；team/A 固定在 tag v1，依赖 team/B@same，B 因此取 tag v1。没有依赖的
        # 本地模块不读取 BSP 分支，B 仍取 v1。
        # The BSP is on master; team/A is fixed at the tag v1 and depends on team/B@same, so
        # B takes the tag v1. A local Module without dependencies does not read the BSP
        # branch, and B still takes v1.
        b = self.upstream("team/B", branches=())
        v1 = run_git(b, "rev-parse", "HEAD")
        run_git(b, "tag", "v1")
        self.commit(b, [], "newer master")
        a = self.upstream("team/A", ["team/B@same"], branches=())
        run_git(a, "tag", "v1")
        run_git(self.root, "init", "-q", "-b", "master")
        self.configure(["team/A@refs/tags/v1"])
        self.assertEqual(self.sync()["modules"]["team/B"]["commit"], v1)
        (self.root / "xrobot.lock").unlink()
        self.local(self.modules / "local/Chassis")
        self.configure(["team/A@refs/tags/v1", {"path": "Modules/local/Chassis"}])
        self.assertEqual(self.sync()["modules"]["team/B"]["commit"], v1)
        self.assertEqual(self.sync(update=[])["modules"]["team/B"]["commit"], v1)
        # 按 lock 检出时，refs/tags/v1 的请求与锁定的 tag 相符。
        # Checking out the lock, the request refs/tags/v1 matches the locked tag.
        self.assertEqual(self.sync(frozen=True)["modules"]["team/B"]["commit"], v1)

    def test_a_commit_request_matches_its_locked_commit_in_any_length(self):
        # lock 记录请求时的写法（7 位）；改写成 12 位的同一提交，lock 仍然相符。
        # The lock records the spelling of the request (7 digits); the same commit written
        # with 12 still matches the lock.
        a = self.upstream("team/A")
        commit = run_git(a, "rev-parse", "HEAD")
        folder = self.local(self.modules / "local/Chassis", [f"team/A@{commit[:7]}"])
        self.configure([{"path": "Modules/local/Chassis"}])
        self.sync()
        self.local(folder, [f"team/A@{commit[:12]}"])
        self.assertEqual(self.sync(frozen=True)["modules"]["team/A"]["commit"], commit)

    def test_a_dependency_of_a_local_module_counts_as_a_request_of_modules_yaml(self):
        # 本地模块请求 team/C@same-or-dev，与在 modules.yaml 中写这一行的结果相同。
        # A local Module that requests team/C@same-or-dev resolves as if modules.yaml had
        # that line.
        b = self.upstream("team/B", branches=())
        run_git(b, "tag", "v1")
        newer = self.commit(b, [], "newer master")
        a = self.upstream("team/A", ["team/B@same"], branches=())
        run_git(a, "tag", "v1")
        self.upstream("team/C")
        run_git(self.root, "init", "-q", "-b", "master")
        self.configure(["team/A@refs/tags/v1", "team/C@same-or-dev"])
        direct = self.sync()["modules"]
        (self.root / "xrobot.lock").unlink()
        self.local(self.modules / "local/Chassis", ["team/C@same-or-dev"])
        self.configure(["team/A@refs/tags/v1", {"path": "Modules/local/Chassis"}])
        through_local = self.sync()["modules"]
        self.assertEqual(direct["team/B"]["commit"], newer)
        self.assertEqual(
            {k: v["commit"] for k, v in through_local.items()},
            {k: v["commit"] for k, v in direct.items()},
        )

    def test_update_outside_git_fails_for_a_local_dependency_that_follows_the_bsp(self):
        self.upstream("team/B")
        self.local(self.modules / "local/Chassis", ["team/B@same-or-dev"])
        self.local(self.modules / "local/Gimbal", ["team/B@same-or-dev"])
        self.configure([{"path": "Modules/local/Chassis"}, {"path": "Modules/local/Gimbal"}])
        self.sync(context_ref="refs/heads/dev")
        with self.assertRaisesRegex(
            ValueError, r"`--update` would resolve team/B@same-or-dev by following the BSP"
        ):
            self.sync(update=[])
        self.sync(update=[], context_ref="refs/heads/dev")

    def test_a_branch_dependency_that_moved_upstream_keeps_its_locked_commit(self):
        # 本地模块新增依赖时重新解析：已锁定、请求未变的 team/B@master 不随上游移动。
        # A local Module gains a dependency and the closure is resolved again: team/B@master,
        # locked and requested the same way, does not move with upstream.
        b = self.upstream("team/B")
        self.upstream("team/C")
        folder = self.local(self.modules / "local/Chassis", ["team/B@master"])
        self.configure([{"path": "Modules/local/Chassis"}])
        locked = self.sync()["modules"]["team/B"]["commit"]
        self.commit(b, [], "master moves")
        self.local(folder, ["team/B@master", "team/C@master"])
        lock = self.sync()
        self.assertEqual(lock["modules"]["team/B"]["commit"], locked)
        self.assertIn("team/C", lock["modules"])

    def test_an_undeclared_local_dependency_fails_on_the_locked_path(self):
        self.local(self.modules / "local/Base")
        top = self.local(self.modules / "local/Top")
        self.configure([{"path": "Modules/local/Top"}, {"path": "Modules/local/Base"}])
        self.sync()
        self.local(top, ["local/Other"])
        for options in ({}, {"frozen": True}):
            with self.assertRaisesRegex(ValueError, "local/Top requests local/Other, which"):
                self.sync(**options)

    def test_a_local_module_reached_twice_warns_once(self):
        self.local(self.tmp / "shared/Base")
        self.local(self.modules / "local/Left", ["local/Base"])
        self.local(self.modules / "local/Right", ["local/Base"])
        self.configure(
            [
                {"path": "Modules/local/Left"},
                {"path": "Modules/local/Right"},
                {"path": "../shared/Base"},
            ]
        )
        _, err = self.sync_warnings()
        self.assertEqual(err.count("local/Base is outside the BSP"), 1)

    def test_a_path_cannot_be_the_bsp_root_modules_or_a_folder_around_the_bsp(self):
        name = self.root.name
        self.write(self.root / f"{name}.hpp", "#pragma once\n" + manifest_block("x"))
        self.write(self.modules / "Modules.hpp", "#pragma once\n" + manifest_block("x"))
        outer = self.root.parent.name
        self.write(self.root.parent / f"{outer}.hpp", "#pragma once\n" + manifest_block("x"))
        for path in (f"../{name}", "Modules", f"../../{outer}"):
            with self.subTest(path):
                self.configure([{"path": path}])
                with self.assertRaisesRegex(ValueError, "is the BSP root, Modules/"):
                    self.sync()
                with self.assertRaisesRegex(ValueError, "is the BSP root, Modules/"):
                    add_module(self.modules / "modules.yaml", local_path=path)

    def test_cmake_receives_quotes_and_dollars_escaped(self):
        folder = self.local(self.tmp / "we ird $dir/a${b}c/Foo", cmake=True)
        self.configure([{"path": folder.as_posix()}])
        self.sync_warnings()
        escaped = folder.resolve().as_posix().replace("$", "\\$")
        self.assertIn(f'include("{escaped}/CMakeLists.txt")', self.cmake())

    def test_a_folder_on_another_drive_is_written_as_an_absolute_path(self):
        self.local(self.tmp / "shared/Gimbal")
        self.configure([{"path": "../shared/Gimbal"}])
        with mock.patch("xrobot.lock.os.path.relpath", side_effect=ValueError("other drive")):
            self.sync_warnings()
        absolute = (self.tmp / "shared/Gimbal").resolve().as_posix()
        self.assertIn(f'target_include_directories(xr PUBLIC "{absolute}")', self.cmake())

    def test_a_linked_modules_folder_keeps_the_bsp_root(self):
        real = self.tmp / "store/Modules"
        real.parent.mkdir(parents=True)
        self.modules.rename(real)
        if os.name == "nt":
            made = subprocess.run(
                ["cmd", "/c", "mklink", "/J", str(self.modules), str(real)], capture_output=True
            ).returncode
        else:
            os.symlink(str(real), str(self.modules), target_is_directory=True)
            made = 0
        if made:
            self.skipTest("a directory link cannot be created here")
        self.local(self.root / "Local/Chassis", cmake=True)
        self.configure([{"path": "Local/Chassis"}])
        lock, err = self.sync_warnings()
        self.assertEqual(err, "")
        self.assertEqual(set(lock["local"]), {"local/Chassis"})
        self.assertIn(
            'include("${CMAKE_CURRENT_LIST_DIR}/../Local/Chassis/CMakeLists.txt")', self.cmake()
        )
        (state,) = locked_modules(self.modules, self.root / "xrobot.lock")
        self.assertEqual((state["status"], state["in_tree"]), ("ok", True))

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
        self.local(self.modules / "local/Chassis")
        self.local(self.root / "x/Chassis")
        out, _ = self.ok("module", "add", "--path", "Modules/local/Chassis")
        self.assertIn("local/Chassis", out)
        requests, _ = read_modules_yaml(self.modules / "modules.yaml")
        self.assertEqual(requests[-1]["path"], "Modules/local/Chassis")
        self.fails("module", "add", "--path", "x/Chassis", pattern="already requested")
        self.ok("module", "remove", "local/Chassis")
        self.assertEqual(read_modules_yaml(self.modules / "modules.yaml")[0], [])

    def test_module_add_path_checks_the_folder(self):
        self.fails("module", "add", "--path", "does/not/Exist", pattern="has no .*Exist.hpp")
        (self.modules / "local/Empty").mkdir(parents=True)
        self.fails("module", "add", "--path", "Modules/local/Empty", pattern="has no .*Empty.hpp")
        self.assertEqual(read_modules_yaml(self.modules / "modules.yaml")[0], [])

    def test_module_add_path_is_taken_from_the_current_directory(self):
        self.local(self.modules / "local/Chassis")
        out, _ = self.ok("module", "add", "--path", "Chassis", cwd=self.modules / "local")
        self.assertIn("path: Modules/local/Chassis", out)
        requests, _ = read_modules_yaml(self.modules / "modules.yaml")
        self.assertEqual(requests[-1]["path"], "Modules/local/Chassis")

    def test_module_add_path_from_another_drive_is_written_absolute(self):
        self.local(self.modules / "local/Chassis")
        with mock.patch("xrobot.cli.os.path.relpath", side_effect=ValueError("other drive")):
            out, _ = self.ok("module", "add", "--path", "Chassis", cwd=self.modules / "local")
        absolute = (self.modules / "local/Chassis").resolve().as_posix()
        self.assertEqual(read_modules_yaml(self.modules / "modules.yaml")[0][-1]["path"], absolute)

    def test_setup_counts_local_modules(self):
        self.local(self.modules / "local/Chassis")
        self.configure([{"path": "Modules/local/Chassis"}])
        # setup 在解析模块之后才检查 User/，这里只看解析的输出。
        # setup checks User/ after resolving the Modules; only the resolution output matters here.
        _, out, _ = self.run_cli("setup")
        self.assertIn("Resolved 0 Module commits and 1 local Module\n", out)

    def test_module_add_takes_a_request_or_a_path(self):
        for argv in (("module", "add"), ("module", "add", "team/A", "--path", "x/A")):
            code, _, err = self.run_cli(*argv)
            self.assertEqual(code, 2, err)
            self.assertIn("request", err)
        self.fails("module", "add", "local/A", pattern="reserved for local Modules")

    def test_add_module_quotes_a_path_when_yaml_needs_it(self):
        path = self.modules / "modules.yaml"
        self.local(self.root / "[a] b/Chassis")
        self.assertEqual(add_module(path, local_path="[a] b/Chassis"), "local/Chassis")
        self.assertEqual(read_modules_yaml(path)[0][-1]["path"], "[a] b/Chassis")
        remove_module(path, "local/Chassis")

    def test_add_module_writes_a_long_path_on_one_line(self):
        path = self.modules / "modules.yaml"
        folder = self.local(
            self.tmp / "My Projects/Robot Firmware 2026/Shared Modules/Chassis IO Modules/ChassisIO"
        )
        long = folder.resolve().as_posix()
        self.assertGreater(len(long), 90)
        add_module(path, local_path=long)
        self.assertEqual(read_modules_yaml(path)[0][-1]["path"], long)
        self.assertIn(f"\n  - path: {long}\n", path.read_text(encoding="utf-8"))


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

    def test_editing_a_local_header_makes_the_generated_header_stale(self):
        self.lock_local("Modules/local/Led")
        self.generate({"modules": [led()]})
        self.assertEqual(self.project.header_state()["status"], "fresh")
        digest = read_header_inputs(self.root / "User/xrobot_main.hpp").digest
        self.write(
            self.root / "Modules/local/Led/Led.hpp",
            "#pragma once" + chr(10) + EMPTY_MANIFEST + LED + chr(10) + "// edited" + chr(10),
        )
        self.assertEqual(self.project.header_state()["status"], "stale")
        self.generate()
        self.assertEqual(self.project.header_state()["status"], "fresh")
        self.assertNotEqual(read_header_inputs(self.root / "User/xrobot_main.hpp").digest, digest)

    def test_describe_marks_local_and_git_modules(self):
        self.module("Motor", "class Motor { public: Motor() {} };")
        self.lock_local("Modules/local/Led")
        modules = {m["id"]: m for m in describe(self.project)["lock"]["modules"]}
        self.assertEqual(
            modules["local/Led"],
            {
                "id": "local/Led",
                "kind": "local",
                "commit": None,
                "head": None,
                "status": "ok",
                "path": "Modules/local/Led",
                "in_tree": True,
            },
        )
        self.assertEqual(modules["team/Motor"]["kind"], "git")
        self.assertNotIn("path", modules["team/Motor"])
        self.assertEqual(modules["team/Motor"]["status"], "ok")
        self.lock_local("../shared/Led")
        modules = {m["id"]: m for m in describe(self.project)["lock"]["modules"]}
        self.assertEqual(
            (modules["local/Led"]["path"], modules["local/Led"]["in_tree"]),
            ("../shared/Led", False),
        )

    def test_an_instance_of_a_local_module_outside_the_bsp(self):
        self.lock_local("../shared/Led")
        code = self.generate({"modules": [led()]})
        self.assertIn('// xrobot: depends "../../shared/Led/Led.hpp"', code)
