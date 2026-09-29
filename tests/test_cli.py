"""The `xrobot` command line (xrobot.cli) and the Module skeleton it creates."""

import contextlib
import io
import json
import os
import subprocess
import sys
from pathlib import Path
from unittest import mock

import yaml
from fixtures import BspTestCase, TempDirTestCase, UpstreamTestCase, manifest_block

from xrobot import __version__
from xrobot.cli import main, parse_value
from xrobot.config import load_config
from xrobot.init_module import read_modules_yaml
from xrobot.module_parser import parse_manifest_from_header, source_interface

REPOSITORY = Path(__file__).resolve().parents[1]
LED = (
    "namespace LibXR { class GPIO; }\nclass Led { public:\n  struct Param { int cycle = 250; };\n"
    "  Led(LibXR::GPIO& gpio, Param param = {}, float gain = 1.0f) {}\n  void OnMonitor() {} };"
)
MAIN = '#include "xrobot_main.hpp"\nint main() { XR_REGISTER(pin, LibXR::GPIO); XROBOT_MAIN(); }\n'


class CliMixin:
    def run_cli(self, *argv, cwd=None):
        out, err = io.StringIO(), io.StringIO()
        previous = os.getcwd()
        os.chdir(str(cwd or self.root))
        try:
            with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
                try:
                    code = main([str(a) for a in argv])
                except SystemExit as exit:
                    code = exit.code
        finally:
            os.chdir(previous)
        return code, out.getvalue(), err.getvalue()

    def ok(self, *argv, cwd=None):
        code, out, err = self.run_cli(*argv, cwd=cwd)
        self.assertEqual(code, 0, out + err)
        return out, err

    def fails(self, *argv, cwd=None, pattern=None):
        code, out, err = self.run_cli(*argv, cwd=cwd)
        self.assertEqual(code, 1, out + err)
        if pattern:
            self.assertRegex(err, pattern)
        return err


class Init(CliMixin, TempDirTestCase):
    def setUp(self):
        super().setUp()
        self.root = self.tmp / "new"
        self.root.mkdir()

    def test_init_creates_a_pinned_bsp_skeleton_once(self):
        out, _ = self.ok("-C", self.root, "init")
        self.assertEqual(
            out.strip(),
            "Created Modules/modules.yaml, Modules/sources.yaml, User/xrobot.yaml, "
            ".gitignore entries",
        )
        self.assertEqual(read_modules_yaml(self.root / "Modules/modules.yaml"), ([], __version__))
        self.assertEqual(
            load_config(self.root / "User/xrobot.yaml"),
            {"modules": [], "settings": {"monitor_sleep_ms": "1000"}},
        )
        sources = yaml.safe_load((self.root / "Modules/sources.yaml").read_text(encoding="utf-8"))
        self.assertEqual(
            [s["url"] for s in sources["sources"]],
            ["https://xrobot.work/xrobot-modules/index.yaml"],
        )
        self.assertEqual(
            (self.root / ".gitignore").read_text(encoding="utf-8").splitlines(),
            ["/User/xrobot_main.hpp", "/Modules/CMakeLists.txt", "/Modules/*/"],
        )
        out, _ = self.ok("-C", self.root, "init")
        self.assertEqual(out.strip(), "Created nothing (already initialized)")

    def test_init_appends_ignore_entries_with_the_file_line_endings(self):
        self.write(".gitignore", "build/\r\n*.o")
        self.ok("-C", self.root, "init")
        self.assertEqual(
            (self.root / ".gitignore").read_bytes(),
            b"build/\r\n*.o\r\n/User/xrobot_main.hpp\r\n/Modules/CMakeLists.txt\r\n/Modules/*/\r\n",
        )

    def test_init_keeps_existing_files_and_ignore_entries(self):
        self.write(".gitignore", "build/\n/Modules/CMakeLists.txt\n")
        self.write("User/xrobot.yaml", "modules: []\n")
        self.ok("-C", self.root, "init")
        self.assertEqual(self.read("User/xrobot.yaml"), "modules: []\n")
        self.assertEqual(
            self.read(".gitignore").splitlines(),
            ["build/", "/Modules/CMakeLists.txt", "/User/xrobot_main.hpp", "/Modules/*/"],
        )

    def test_version(self):
        out, _ = self.ok("--version")
        self.assertEqual(out.strip(), "xrobot " + __version__)

    def test_help_describes_every_command_and_action(self):
        out, _ = self.ok("source", "--help")
        for action in (
            "list",
            "search",
            "get",
            "find",
            "create-sources",
            "add-source",
            "create-index",
            "add-index",
        ):
            self.assertIn(action, out)
        out, _ = self.ok("instance", "set", "--help")
        self.assertIn("VALUE is JSON; text that is not JSON is taken as C++ text.", out)
        code, _, err = self.run_cli("source")
        self.assertEqual(code, 2)
        self.assertIn("the following arguments are required: <action>", err)

    def test_source_needs_a_bsp_or_sources(self):
        if any((p / "Modules/modules.yaml").is_file() for p in self.tmp.parents):
            self.skipTest("a directory above the temporary directory is itself a BSP")
        self.fails("source", "list", cwd=self.root, pattern="No XRobot BSP found")
        self.fails(
            "source",
            "--sources",
            self.root / "missing.yaml",
            "list",
            cwd=self.root,
            pattern="missing.yaml does not exist; run `xrobot source create-sources`",
        )

    def test_commands_outside_a_bsp_fail_with_a_hint(self):
        if any((p / "Modules/modules.yaml").is_file() for p in self.tmp.parents):
            self.skipTest("a directory above the temporary directory is itself a BSP")
        self.fails("gen", cwd=self.root, pattern=r"No XRobot BSP found at or above .*xrobot init")
        self.assertFalse((self.root / "Modules").exists())


class Commands(CliMixin, BspTestCase):
    def setUp(self):
        super().setUp()
        self.module("Led", LED)
        self.entry(MAIN)
        self.config(
            {
                "modules": [
                    {
                        "module": "Led",
                        "id": "led",
                        "args": [{"gpio": "pin"}, {"param": {"cycle": "100"}}, {"gain": "1.0f"}],
                    }
                ]
            }
        )
        (self.root / "User/products").mkdir()

    def test_the_root_is_found_from_a_subdirectory_or_with_C(self):
        out, _ = self.ok("gen", cwd=self.root / "User/products")
        self.assertEqual(out.strip(), "Generated User/xrobot_main.hpp for User/xrobot.yaml")
        (self.root / "User/xrobot_main.hpp").unlink()
        self.ok("-C", self.root / "User/products", "gen", cwd=self.tmp)
        self.assertTrue((self.root / "User/xrobot_main.hpp").is_file())

    def test_gen_selects_a_product_relative_to_the_cwd_or_the_root(self):
        self.config({"modules": []}, name="products/alt.yaml")
        out, _ = self.ok("gen", "-c", "alt.yaml", cwd=self.root / "User/products")
        self.assertEqual(out.strip(), "Generated User/xrobot_main.hpp for User/products/alt.yaml")
        self.assertIn('// xrobot: config "products/alt.yaml"', self.read("User/xrobot_main.hpp"))
        self.ok("gen", "-c", "User/xrobot.yaml", cwd=self.root / "User/products")
        self.assertIn('// xrobot: config "xrobot.yaml"', self.read("User/xrobot_main.hpp"))
        out, _ = self.ok("gen", cwd=self.root)
        self.assertIn("User/xrobot.yaml", out)

    def test_a_renamed_selected_product_must_be_selected_again(self):
        self.config({"modules": []}, name="products/alt.yaml")
        self.ok("gen", "-c", "User/products/alt.yaml", cwd=self.root)
        (self.root / "User/products/alt.yaml").rename(self.root / "User/products/renamed.yaml")
        self.fails(
            "gen",
            cwd=self.root,
            pattern="generated for User/products/alt.yaml, which does not exist",
        )
        out, _ = self.ok("gen", "-c", "User/products/renamed.yaml", cwd=self.root)
        self.assertEqual(
            out.strip(), "Generated User/xrobot_main.hpp for User/products/renamed.yaml"
        )

    def test_errors_exit_with_status_1_and_a_message(self):
        self.fails("gen", "-c", "User/missing.yaml", pattern="User/missing.yaml does not exist")
        self.config({"modules": [{"module": "Led", "id": "led", "args": [{"gpio": "other"}]}]})
        self.fails("gen", pattern="User/xrobot.yaml: led: named arguments")
        self.config("modules: [\n")
        self.fails("gen", pattern=r"User/xrobot.yaml:\d+: YAML syntax error")
        self.assertFalse((self.root / "User/xrobot_main.hpp").exists())

    def test_output_is_utf8_whatever_the_platform_encoding(self):
        header = self.write(
            self.tmp / "Zh/Zh.hpp",
            "#pragma once\n" + manifest_block("闪烁") + "class Zh { public: Zh() {} };\n",
        )
        out = io.TextIOWrapper(io.BytesIO(), encoding="gbk")
        err = io.TextIOWrapper(io.BytesIO(), encoding="gbk")
        with mock.patch.object(sys, "stdout", out), mock.patch.object(sys, "stderr", err):
            self.assertEqual(main(["module", "show", str(header)]), 0)
            self.assertEqual(main(["-C", str(self.root), "gen", "-c", "User/缺失.yaml"]), 1)
            out.flush()
            err.flush()
        self.assertIn("闪烁".encode(), out.buffer.getvalue())
        self.assertIn("User/缺失.yaml does not exist".encode(), err.buffer.getvalue())

    def test_describe_prints_json(self):
        out, _ = self.ok("describe", cwd=self.root / "User")
        result = json.loads(out)
        self.assertEqual((result["schema"], result["config"]), (1, "User/xrobot.yaml"))
        self.assertEqual(result["instances"][0]["id"], "led")

    def test_instance_editing(self):
        out, _ = self.ok("instance", "add", "Led", "--id", "second")
        self.assertIn("Added second to User/xrobot.yaml; fill the null values", out)
        self.ok("instance", "set", "second", "args.gpio", '"pin"')
        self.ok("instance", "set", "second", "args.gain", "2.5")
        second = load_config(self.root / "User/xrobot.yaml")["modules"][1]
        self.assertEqual(
            second["args"], [{"gpio": "pin"}, {"param": {"cycle": "250"}}, {"gain": "2.5"}]
        )
        self.fails(
            "instance",
            "set",
            "second",
            "args.gain",
            "3",
            "--if-match",
            "0" * 64,
            pattern="changed since it was read",
        )
        self.ok("instance", "rename", "second", "backup")
        self.ok("instance", "remove", "backup")
        self.assertEqual(
            [i["id"] for i in load_config(self.root / "User/xrobot.yaml")["modules"]], ["led"]
        )

    def test_values_that_are_not_json_are_cpp_text(self):
        self.assertEqual(parse_value('"LED_B"'), "LED_B")
        self.assertEqual(parse_value('{"a": "1"}'), {"a": "1"})
        self.assertEqual(parse_value("2.0f"), "2.0f")
        self.ok("instance", "set", "led", "args.gain", "2.0f")
        self.ok("instance", "set", "led", "args.param", "{250}")
        self.assertEqual(
            load_config(self.root / "User/xrobot.yaml")["modules"][0]["args"],
            [{"gpio": "pin"}, {"param": "{250}"}, {"gain": "2.0f"}],
        )

    def test_instance_add_writes_to_the_selected_product(self):
        self.config({"modules": []}, name="products/alt.yaml")
        self.ok("gen", "-c", "User/products/alt.yaml")
        self.ok("instance", "add", "Led")
        self.assertEqual(
            [i["id"] for i in load_config(self.root / "User/products/alt.yaml")["modules"]],
            ["led_0"],
        )
        self.ok("instance", "-c", "User/xrobot.yaml", "add", "Led")
        self.assertEqual(
            [i["id"] for i in load_config(self.root / "User/xrobot.yaml")["modules"]],
            ["led", "led_0"],
        )

    def test_module_requests(self):
        out, _ = self.ok("module", "add", "team/Other")
        self.assertIn("Added team/Other; run `xrobot setup` to fetch it", out)
        self.assertEqual(
            read_modules_yaml(self.root / "Modules/modules.yaml"),
            ([{"id": "team/Other", "ref": "same-or-dev"}], __version__),
        )
        self.fails("module", "add", "team/Other", pattern="already requested")
        self.ok("module", "remove", "team/Other")
        self.assertEqual(read_modules_yaml(self.root / "Modules/modules.yaml")[0], [])

    def test_module_show_prints_the_manifest_and_constructors(self):
        for target in (self.root / "Modules/team/Led", "."):
            with self.subTest(target=str(target)):
                out, _ = self.ok("module", "show", target, cwd=self.root / "Modules/team/Led")
                self.assertIn("{}", out)
                self.assertRegex(
                    out,
                    r"Led\.hpp:5: Led\(LibXR::GPIO& gpio, Param param = \{\}, float gain = 1\.0f\)",
                )

    def test_module_show_takes_the_id_of_a_locked_module(self):
        for target in ("team/Led", "Led"):
            with self.subTest(target=target):
                out, _ = self.ok("module", "show", target, cwd=self.root / "User")
                self.assertRegex(out, r"Led\.hpp:5: Led\(LibXR::GPIO& gpio")
        self.fails(
            "module",
            "show",
            "team/Missing",
            pattern="team/Missing: not a file or folder; Module not found: team/Missing",
        )

    def test_format_check_and_rewrite(self):
        self.config(
            "modules:\n- module: Led\n  id: led\n  args:\n  - gpio: pin\n  - param: {cycle: 1}\n  - gain: 1.0f\n"
        )
        self.fails(
            "format",
            "--check",
            pattern="Found 1 file not in the canonical layout; run `xrobot format`",
        )
        out, _ = self.run_cli("format", "--check")[1:]
        self.assertIn("needs formatting: User/xrobot.yaml", out)
        out, _ = self.ok("format")
        self.assertEqual(out.strip(), "formatted: User/xrobot.yaml")
        self.ok("format", "--check")
        self.assertIn("\n  - module: Led\n", self.read("User/xrobot.yaml"))

    def test_sync_prints_the_diff(self):
        self.module("Led", LED.replace("int cycle = 250;", "int cycle = 250; int phase = 0;"))
        out, _ = self.ok("sync")
        self.assertIn("+++ User/xrobot.yaml", out)
        self.assertIn("+          phase:", out.replace("'0'", "").replace(" 0", ""))
        self.assertEqual(self.ok("sync")[0], "")

    def test_new_module_skeleton(self):
        out, _ = self.ok(
            "new-module",
            "Blink",
            "--desc",
            "Blinks a pin",
            "--constructor",
            "LibXR::GPIO& gpio",
            "--constructor",
            "int period_ms = 500",
            "--depends",
            "team/Timer",
            "team/Log@v1",
            "--include",
            "gpio.hpp",
            "--out",
            self.tmp / "out",
        )
        folder = self.tmp / "out/Blink"
        self.assertEqual(out.strip(), f"Created {folder}")
        manifest = parse_manifest_from_header(folder / "Blink.hpp")
        self.assertEqual(manifest.description, "Blinks a pin")
        self.assertEqual(
            manifest.depends,
            [{"id": "team/Timer", "ref": "same-or-dev"}, {"id": "team/Log", "ref": "v1"}],
        )
        interface = source_interface(folder / "Blink.hpp")
        self.assertEqual(
            [p["declaration"] for p in interface["constructors"][0]["arguments"]],
            ["LibXR::GPIO& gpio", "int period_ms = 500"],
        )
        self.assertIn('#include "gpio.hpp"', (folder / "Blink.hpp").read_text(encoding="utf-8"))
        self.assertIn(
            'target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")',
            (folder / "CMakeLists.txt").read_text(encoding="utf-8"),
        )
        workflow = yaml.safe_load(
            (folder / ".github/workflows/build.yml").read_text(encoding="utf-8")
        )
        job = workflow["jobs"]["build"]
        self.assertEqual(job["uses"], "xrobot-org/XRobot/.github/workflows/module-ci.yml@v1")
        self.assertEqual(job["with"], {"template-args": "[]"})
        self.assertIn(
            "xrobot module add <owner>/Blink", (folder / "README.md").read_text(encoding="utf-8")
        )

    def test_new_module_refuses_bad_input_without_creating_anything(self):
        out = self.tmp / "out"
        self.fails(
            "new-module",
            "Blink",
            "--depends",
            "Timer",
            "--out",
            out,
            pattern="Expected canonical owner/repo",
        )
        self.fails(
            "new-module", "1Blink", "--out", out, pattern="Module name must be a C\\+\\+ identifier"
        )
        self.assertFalse((out / "Blink").exists())
        self.ok("new-module", "Blink", "--out", out)
        self.fails(
            "new-module", "Blink", "--out", out, pattern="Refusing to overwrite existing module"
        )


class SharedModuleWorkflow(TempDirTestCase):
    def test_the_reusable_workflow_accepts_what_module_repositories_pass(self):
        from xrobot.module_creator import CI_WORKFLOW

        shared = yaml.safe_load(
            (REPOSITORY / ".github/workflows/module-ci.yml").read_text(encoding="utf-8")
        )
        trigger = shared.get("on", shared.get(True))
        inputs = trigger["workflow_call"]["inputs"]
        caller = yaml.safe_load(CI_WORKFLOW)
        self.assertTrue(set(caller["jobs"]["build"]["with"]) <= set(inputs))
        for name in ("xrobot-ref", "libxr-ref", "dependency-ref", "template-args"):
            self.assertIn(name, inputs)
        self.assertEqual(inputs["xrobot-ref"]["default"], "master")
        self.assertEqual(inputs["libxr-ref"]["default"], "master")
        steps = "\n".join(str(step.get("run", "")) for step in shared["jobs"]["build"]["steps"])
        self.assertIn('xrobot check-module "$XR_MODULE_ID"', steps)
        self.assertIn("add_library(module_check OBJECT module_check.cpp)", steps)
        self.assertNotRegex(steps, r"git (tag|push)")


class ModuleCiPreparation(CliMixin, UpstreamTestCase):
    """The shared workflow's preparation script, run against local repositories."""

    def test_the_pull_request_head_is_probed_with_dependencies_from_the_context(self):
        from fixtures import run_git

        b = self.upstream("team/B")
        a = self.upstream("team/A", ["team/B@same-or-dev"], listed=False)
        selected = run_git(a, "rev-parse", "HEAD")
        local = self.modules / "team/A"
        run_git(None, "clone", "-q", str(a), str(local))
        run_git(local, "checkout", "-q", "--detach", selected)
        self.commit(a, ["team/B@same-or-dev"], "new remote head")
        (self.modules / "modules.yaml").unlink()
        (self.modules / "sources.yaml").unlink()
        shared = yaml.safe_load(
            (REPOSITORY / ".github/workflows/module-ci.yml").read_text(encoding="utf-8")
        )
        step = next(
            s for s in shared["jobs"]["build"]["steps"] if "PYCODE" in str(s.get("run", ""))
        )
        lines = step["run"].splitlines()
        start = next(i for i, line in enumerate(lines) if "<<'PYCODE'" in line) + 1
        end = next(i for i in range(start, len(lines)) if lines[i].strip() == "PYCODE")
        indent = min(len(text) - len(text.lstrip()) for text in lines[start:end] if text.strip())
        script = "\n".join(text[indent:] for text in lines[start:end])

        class Index:
            def __init__(self, data):
                self.text = yaml.safe_dump(data)

            def raise_for_status(self):
                pass

        def index(url, **kwargs):
            if url == "https://xrobot.work/xrobot-modules/index.yaml":
                return Index(
                    {
                        "packages": [
                            {
                                "id": "team/A",
                                "type": "module",
                                "repo": (self.tmp / "wrong-upstream").as_uri(),
                            },
                            {"id": "team/B", "type": "module", "repo": b.as_uri()},
                        ]
                    }
                )
            if url == "https://qdu-robomaster.github.io/qdu-future-modules/index.yaml":
                return Index({"modules": []})
            raise AssertionError("unexpected index request: " + url)

        previous = os.getcwd()
        os.chdir(str(self.root))
        try:
            with mock.patch.dict(
                os.environ, XR_MODULE_ID="team/A", XR_DEPENDENCY_REF="refs/heads/feature/ci"
            ):
                exec(compile(script, "<module CI preparation>", "exec"), {"__name__": "__ci__"})
        finally:
            os.chdir(previous)
        with mock.patch("xrobot.source_manager.requests.get", side_effect=index):
            self.ok("check-module", "team/A", "-o", self.root / "module_check.cpp")
        lock = yaml.safe_load((self.root / "xrobot.lock").read_text(encoding="utf-8"))
        self.assertEqual(lock["modules"]["team/A"]["commit"], selected)
        self.assertEqual(lock["modules"]["team/B"]["resolved_ref"], "dev")
        self.assertEqual(run_git(local, "rev-parse", "HEAD"), selected)
        probe = (self.root / "module_check.cpp").read_text(encoding="utf-8")
        self.assertIn("void XRobotCompileCheck()", probe)
        self.assertIn("static A module_0;", probe)


class Setup(CliMixin, UpstreamTestCase):
    def setUp(self):
        super().setUp()
        self.led = self.upstream("team/Led")
        self.commit(self.led, [], "led")
        self.configure(["team/Led@master"])
        self.write(
            self.root / "User/app_main.cpp",
            '#include "xrobot_main.hpp"\nint main() { XROBOT_MAIN(); }\n',
        )
        self.write(self.root / "User/xrobot.yaml", "modules:\n  - module: team/Led\n    id: led\n")

    def test_setup_resolves_checks_every_config_and_generates_the_selected_one(self):
        self.write(self.root / "User/products/alt.yaml", "modules: []\n")
        out, err = self.ok("setup", cwd=self.root / "User")
        self.assertIn("Resolved 1 Module commit\n", out)
        self.assertIn("Checked 2 configs; generated User/xrobot_main.hpp for User/xrobot.yaml", out)
        self.assertEqual(err, "")
        self.assertTrue((self.root / "xrobot.lock").is_file())
        self.assertIn(
            "static Led led;", (self.root / "User/xrobot_main.hpp").read_text(encoding="utf-8")
        )
        self.ok("gen", "-c", "User/products/alt.yaml")
        self.ok("setup", "--frozen")
        self.assertIn(
            '// xrobot: config "products/alt.yaml"',
            (self.root / "User/xrobot_main.hpp").read_text(encoding="utf-8"),
        )

    def test_setup_fails_when_any_config_is_invalid(self):
        self.write(
            self.root / "User/products/a.yaml", "modules:\n  - {module: team/Led, id: Led}\n"
        )
        self.write(
            self.root / "User/products/b.yaml", "modules:\n  - {module: team/Missing, id: m}\n"
        )
        err = self.fails("setup")
        self.assertIn("User/products/a.yaml: Led: instance id Led is also a class name", err)
        self.assertIn("User/products/b.yaml: m: Module not found: team/Missing", err)
        self.assertFalse((self.root / "User/xrobot_main.hpp").exists())

    def test_frozen_or_offline_without_a_lock_fails(self):
        for option in ("--frozen", "--offline"):
            self.fails(
                "setup",
                option,
                pattern="xrobot.lock does not exist; run `xrobot setup` once without "
                "--frozen or --offline",
            )

    def test_a_git_timeout_is_reported_without_a_traceback(self):
        timeout = subprocess.TimeoutExpired(["git", "fetch"], 300)
        with mock.patch("xrobot.init_module.subprocess.run", side_effect=timeout):
            self.fails("setup", pattern=r"Git did not finish within 300 s")

    def test_a_different_tool_pin_is_a_warning_and_an_error_when_frozen(self):
        self.configure(["team/Led@master"], pin="0.9.0")
        _, err = self.ok("setup")
        warning = f"warning: installed XRobot {__version__} differs from the pinned 0.9.0"
        self.assertIn(warning, err)
        _, err = self.ok("gen")
        self.assertIn(warning, err)
        self.fails(
            "setup",
            "--frozen",
            pattern=r"differs from the pinned 0\.9\.0 \(--frozen requires the pinned version\)",
        )
        self.configure(["team/Led@master"], pin="0123456789abcdef0123456789abcdef01234567")
        _, err = self.ok("setup", "--frozen")
        self.assertEqual(err, "")
        self.configure(["team/Led@master"], pin=None)
        _, err = self.ok("setup")
        self.assertIn(
            f"warning: Modules/modules.yaml does not pin XRobot; add `xrobot: {__version__}`", err
        )

    def test_setup_update_syncs_config_fields(self):
        self.ok("setup")
        self.write(
            self.led / "Led.hpp",
            "#pragma once\nclass Led { public: explicit Led(int period = 5) {} };\n",
        )
        from fixtures import run_git

        run_git(self.led, "commit", "-q", "-am", "new parameter")
        out, _ = self.ok("setup", "--update")
        self.assertIn("+      - period: 5", out.replace("'5'", "5"))
        self.assertEqual(
            load_config(self.root / "User/xrobot.yaml")["modules"][0]["args"], [{"period": "5"}]
        )

    def test_check_module_writes_a_probe_offline(self):
        self.ok("setup")
        out, _ = self.ok("check-module", "team/Led", "-o", self.tmp / "probe.cpp", "--offline")
        self.assertIn("Generated", out)
        self.assertIn("static Led module_0;", (self.tmp / "probe.cpp").read_text(encoding="utf-8"))

    def test_source_queries(self):
        out, _ = self.ok("source", "list")
        self.assertIn("team/Led [module]", out)
        out, _ = self.ok("source", "get", "Led")
        self.assertEqual(yaml.safe_load(out)["id"], "team/Led")

    def test_source_finds_the_bsp_like_other_commands(self):
        out, _ = self.ok("source", "search", "led", cwd=self.root / "User")
        self.assertIn("team/Led [module]", out)
        out, _ = self.ok("-C", self.root / "User", "source", "list", cwd=self.tmp)
        self.assertIn("team/Led [module]", out)

    def test_source_options_are_passed_through(self):
        out, _ = self.ok(
            "source", "--sources", self.root / "Modules/sources.yaml", "list", cwd=self.tmp
        )
        self.assertIn("team/Led [module]", out)
