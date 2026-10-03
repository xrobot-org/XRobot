"""共享工作流：模块 CI（.github/workflows/module-ci.yml）和 STM32 BSP CI
（.github/workflows/bsp-stm32-ci.yml）。
The shared workflows: Module CI (.github/workflows/module-ci.yml) and STM32 BSP CI
(.github/workflows/bsp-stm32-ci.yml).
"""

import contextlib
import io
import json
import os
import shutil
import subprocess
import unittest
from pathlib import Path
from unittest import mock

import yaml
from fixtures import CliMixin, TempDirTestCase, UpstreamTestCase

REPOSITORY = Path(__file__).resolve().parents[1]


class SharedModuleWorkflow(TempDirTestCase):
    """模块仓库传入的参数是共享工作流接受的输入。
    What Module repositories pass are inputs the shared workflow accepts.
    """

    def test_the_reusable_workflow_accepts_what_module_repositories_pass(self):
        from xrobot.module_creator import ci_workflow

        shared = yaml.safe_load(
            (REPOSITORY / ".github/workflows/module-ci.yml").read_text(encoding="utf-8")
        )
        trigger = shared.get("on", shared.get(True))
        inputs = trigger["workflow_call"]["inputs"]
        caller = yaml.safe_load(ci_workflow([]))
        self.assertTrue(set(caller["jobs"]["build"]["with"]) <= set(inputs))
        # 模板实参是一个 JSON 列表，放在单引号 YAML 字符串中。
        # The template arguments are a JSON list inside a single-quoted YAML string.
        values = ["Frame{.name = 'a'}", "3"]
        caller = yaml.safe_load(ci_workflow(values))
        self.assertEqual(json.loads(caller["jobs"]["build"]["with"]["template-args"]), values)
        for name in ("xrobot-ref", "libxr-ref", "dependency-ref", "template-args"):
            self.assertIn(name, inputs)
        self.assertEqual(inputs["xrobot-ref"]["default"], "master")
        self.assertEqual(inputs["libxr-ref"]["default"], "master")
        steps = "\n".join(str(step.get("run", "")) for step in shared["jobs"]["build"]["steps"])
        self.assertIn('xrobot check-module "$XR_MODULE_ID"', steps)
        self.assertIn("add_library(module_check OBJECT module_check.cpp)", steps)
        self.assertNotRegex(steps, r"git (tag|push)")


class ModuleCiPreparation(CliMixin, UpstreamTestCase):
    """工作流的准备脚本在本地仓库上运行。
    The workflow's preparation script, run against local repositories.
    """

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
                        ]
                    }
                )
            if url == "https://qdu-robomaster.github.io/qdu-future-modules/index.yaml":
                return Index({"modules": []})
            # team/B 只在调用方通过 sources 输入给出的 index 中。
            # team/B is only in the index the caller passes through the sources input.
            if url == "https://example.com/team/index.yaml":
                return Index({"packages": [{"id": "team/B", "type": "module", "repo": b.as_uri()}]})
            raise AssertionError("unexpected index request: " + url)

        previous = os.getcwd()
        os.chdir(str(self.root))
        try:
            with mock.patch.dict(
                os.environ,
                XR_MODULE_ID="team/A",
                XR_DEPENDENCY_REF="refs/heads/feature/ci",
                XR_SOURCES="\n  https://example.com/team/index.yaml\n",
            ):
                exec(compile(script, "<module CI preparation>", "exec"), {"__name__": "__ci__"})
        finally:
            os.chdir(previous)
        sources = yaml.safe_load((self.modules / "sources.yaml").read_text(encoding="utf-8"))
        self.assertEqual(
            [(s["url"], s["priority"]) for s in sources["sources"]],
            [
                ("https://xrobot.work/xrobot-modules/index.yaml", 0),
                ("https://qdu-robomaster.github.io/qdu-future-modules/index.yaml", 0),
                ("https://example.com/team/index.yaml", 1),
                ("../ci-index.yaml", -100),
            ],
        )
        with mock.patch("requests.get", side_effect=index):
            self.ok("check-module", "team/A", "-o", self.root / "module_check.cpp")
        lock = yaml.safe_load((self.root / "xrobot.lock").read_text(encoding="utf-8"))
        self.assertEqual(lock["modules"]["team/A"]["commit"], selected)
        self.assertEqual(lock["modules"]["team/B"]["resolved_ref"], "dev")
        self.assertEqual(run_git(local, "rev-parse", "HEAD"), selected)
        probe = (self.root / "module_check.cpp").read_text(encoding="utf-8")
        self.assertIn("void XRobotCompileCheck()", probe)
        self.assertIn("static A module_0;", probe)


def usable_bash():
    """能运行 git 和 awk 的 bash 是否可用（Windows 上的 bash 可能是没有安装发行版的 WSL）。
    Whether a bash that can run git and awk is available (on Windows, bash may be a WSL
    without a distribution).
    """
    if not shutil.which("bash"):
        return False
    try:
        result = subprocess.run(
            ["bash", "-c", "command -v git awk"], capture_output=True, timeout=60
        )
    except (OSError, subprocess.TimeoutExpired):
        return False
    return result.returncode == 0


def embedded_script(step, marker):
    """步骤的 run 中 <<'marker' 与 marker 之间的脚本，去掉缩进。
    The script between <<'marker' and marker in a step's run, without its indentation.
    """
    lines = step["run"].splitlines()
    start = next(i for i, line in enumerate(lines) if f"<<'{marker}'" in line) + 1
    end = next(i for i in range(start, len(lines)) if lines[i].strip() == marker)
    indent = min(len(text) - len(text.lstrip()) for text in lines[start:end] if text.strip())
    return "\n".join(text[indent:] for text in lines[start:end])


def load_bsp_workflow():
    """共享 BSP CI 工作流的内容。
    The content of the shared BSP CI workflow.
    """
    return yaml.safe_load(
        (REPOSITORY / ".github/workflows/bsp-stm32-ci.yml").read_text(encoding="utf-8")
    )


class SharedBspWorkflow(TempDirTestCase):
    """STM32 BSP 只需写 project 和 configs 的共享工作流。
    The shared workflow of STM32 BSPs, which need only project and configs.
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp
        self.shared = load_bsp_workflow()
        self.steps = {
            name: [s.get("name") for s in job.get("steps", [])]
            for name, job in self.shared["jobs"].items()
        }

    def inputs(self):
        """工作流接受的输入。
        The inputs the workflow accepts.
        """
        trigger = self.shared.get("on", self.shared.get(True))
        return trigger["workflow_call"]["inputs"]

    def plan(self, configs, presets=""):
        """用这些输入运行 plan 作业的脚本，返回输出的构建列表和发布列表。
        Run the script of the plan job with these inputs and return the builds and releases.
        """
        step = next(s for s in self.shared["jobs"]["plan"]["steps"] if s.get("id") == "plan")
        script = embedded_script(step, "PYCODE")
        output = self.tmp / "github_output"
        output.write_text("", encoding="utf-8")
        environment = {"XR_CONFIGS": configs, "XR_PRESETS": presets, "GITHUB_OUTPUT": str(output)}
        with mock.patch.dict(os.environ, environment):
            exec(compile(script, "<plan>", "exec"), {"__name__": "__ci__"})
        values = dict(
            line.split("=", 1) for line in output.read_text(encoding="utf-8").split("\n") if line
        )
        return json.loads(values["builds"]), json.loads(values["releases"])

    def test_a_bsp_needs_only_project_and_configs(self):
        inputs = self.inputs()
        self.assertEqual(
            [name for name, spec in inputs.items() if spec.get("required")], ["project"]
        )
        self.assertEqual(inputs["configs"]["default"], "default")
        self.assertEqual(inputs["config-dir"]["default"], "User/RobotConfig")
        self.assertEqual(inputs["toolchain"]["default"], "cmake/starm-clang.cmake")
        self.assertEqual(inputs["build-type"]["default"], "Release")
        self.assertEqual(inputs["presets"]["default"], "")
        self.assertEqual(inputs["image"]["default"], "ghcr.io/xrobot-org/docker-image-stm32:main")
        # 每个 BSP 的调用只传这个工作流接受的输入。
        # What a BSP passes is what the workflow accepts.
        caller = yaml.safe_load(
            "build:\n  uses: xrobot-org/XRobot/.github/workflows/bsp-stm32-ci.yml@v1\n"
            "  with:\n    project: DevC\n    configs: |\n      default\n      hero\n"
        )
        self.assertTrue(set(caller["build"]["with"]) <= set(inputs))

    def test_the_context_and_release_refs_are_the_expressions_of_the_bsp_workflows(self):
        env = self.shared["env"]
        self.assertEqual(
            env["XR_CONTEXT_REF"],
            "${{ github.ref_type == 'tag' && format('refs/tags/{0}', github.ref_name) || "
            "format('refs/heads/{0}', github.head_ref || github.ref_name) }}",
        )
        self.assertEqual(
            env["XR_RELEASE_REF"],
            "${{ github.base_ref && format('refs/heads/{0}', github.base_ref) || "
            "(github.ref_type == 'tag' && format('refs/tags/{0}', github.ref_name) || "
            "format('refs/heads/{0}', github.ref_name)) }}",
        )

    def test_every_check_and_build_step_of_the_bsp_workflows_is_kept(self):
        # 检查只跑一次；每个构建解析锁定的模块，配置，构建，打包并上传。
        # The checks run once; every build resolves the locked Modules, configures, builds,
        # packages and uploads.
        self.assertEqual(
            self.steps["check"],
            [
                "Checkout",
                "Init submodules",
                "Install pinned XRobot tools",
                "Regenerate BSP objects",
                "Check generated BSP objects are committed",
                "Check line endings",
                "Check config layout",
                "Resolve locked Modules and check every config",
            ],
        )
        self.assertEqual(
            self.steps["build"],
            [
                "Checkout",
                "Init submodules",
                "Install pinned XRobot tools",
                "Resolve locked Modules",
                "Configure",
                "Build",
                "Package firmware",
                "Upload firmware artifact",
            ],
        )
        check = "\n".join(str(s.get("run", "")) for s in self.shared["jobs"]["check"]["steps"])
        self.assertIn("xrobot format --check", check)
        self.assertIn(
            'xrobot setup --frozen --context-ref "$XR_CONTEXT_REF" --release-ref "$XR_RELEASE_REF"',
            check,
        )
        self.assertIn("libxr gen -i .ci-tools/cubemx.yaml -o User/app_main.cpp --xrobot", check)
        build = "\n".join(str(s.get("run", "")) for s in self.shared["jobs"]["build"]["steps"])
        self.assertIn('xrobot gen -c "$XR_CONFIG_DIR/$XR_CONFIG.yaml"', build)
        self.assertIn('cmake --build --preset "$XR_PRESET"', build)
        self.assertIn("-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -Bbuild -G Ninja", build)

    def test_the_default_configuration_is_built_but_not_packaged_or_released(self):
        packaging = [
            s
            for s in self.shared["jobs"]["build"]["steps"]
            if s["name"].startswith(("Package", "Upload"))
        ]
        self.assertEqual([s["if"] for s in packaging], ["matrix.config != 'default'"] * 2)
        release = self.shared["jobs"]["release"]
        self.assertEqual(release["permissions"], {"contents": "write"})
        self.assertEqual(release["needs"], ["plan", "check", "build"])
        self.assertIn("github.event_name == 'release'", release["if"])
        self.assertIn("startsWith(github.ref, 'refs/tags/v')", release["if"])
        self.assertEqual(self.shared["permissions"], {"contents": "read"})

    def test_inputs_reach_the_scripts_through_the_environment(self):
        # 输入不展开到 shell 命令里。
        # Inputs are never expanded into shell commands.
        for name, job in self.shared["jobs"].items():
            for step in job["steps"]:
                self.assertNotIn("${{", str(step.get("run", "")), (name, step.get("name")))

    def test_the_plan_lists_one_build_per_configuration(self):
        builds, releases = self.plan("default\n\n  hero  \nsentry\nhero\n")
        self.assertEqual(
            builds,
            [
                {"config": "default", "preset": "", "name": "default"},
                {"config": "hero", "preset": "", "name": "hero"},
                {"config": "sentry", "preset": "", "name": "sentry"},
            ],
        )
        # 默认配置不打包，也不发布。
        # The default configuration is neither packaged nor released.
        self.assertEqual([b["name"] for b in releases], ["hero", "sentry"])
        self.assertEqual(self.plan("default")[1], [])
        self.assertEqual(self.plan("")[0], [{"config": "default", "preset": "", "name": "default"}])

    def test_the_plan_crosses_configurations_with_presets(self):
        builds, releases = self.plan("default\nhero", "app\nbootloader")
        self.assertEqual(
            [b["name"] for b in builds],
            ["default-app", "default-bootloader", "hero-app", "hero-bootloader"],
        )
        self.assertEqual([b["preset"] for b in builds], ["app", "bootloader"] * 2)
        self.assertEqual([b["name"] for b in releases], ["hero-app", "hero-bootloader"])
        self.assertEqual([b["name"] for b in self.plan("default", "app")[0]], ["default-app"])

    def test_the_plan_refuses_names_that_are_not_plain(self):
        for configs, presets in (("a b", ""), ("hero;rm", ""), ("$(id)", ""), ("default", "x/y")):
            printed = io.StringIO()
            with (
                self.subTest(configs=configs, presets=presets),
                contextlib.redirect_stdout(printed),
                self.assertRaises(SystemExit),
            ):
                self.plan(configs, presets)
            self.assertIn("is not a plain name", printed.getvalue())

    @unittest.skipUnless(usable_bash(), "needs bash, git and awk")
    def test_the_line_ending_check_fails_for_text_stored_with_crlf(self):
        step = next(
            s
            for s in self.shared["jobs"]["check"]["steps"]
            if s.get("name") == "Check line endings"
        )
        repo = self.tmp / "repo"
        repo.mkdir()
        options = ["-c", "core.autocrlf=false", "-c", "commit.gpgsign=false"]

        def git(*args):
            """在临时仓库里运行 git。
            Run git in the temporary repository.
            """
            subprocess.run(["git", *options, *args], cwd=repo, check=True, capture_output=True)

        def check():
            """运行检查步骤，返回退出码和输出。
            Run the check step and return its exit code and output.
            """
            result = subprocess.run(
                ["bash", "-c", step["run"]], cwd=repo, capture_output=True, text=True
            )
            return result.returncode, result.stdout + result.stderr

        git("init", "-q")
        (repo / "lf.c").write_bytes(b"int a;\nint b;\n")
        (repo / "image.bin").write_bytes(b"\x00\x01\r\n\x02")
        git("add", ".")
        self.assertEqual(check()[0], 0)
        (repo / "crlf.c").write_bytes(b"int a;\r\nint b;\r\n")
        git("add", ".")
        code, output = check()
        self.assertEqual(code, 1)
        self.assertIn("crlf.c", output)
        self.assertNotIn("lf.c\n", output.replace("crlf.c", ""))
        self.assertIn("git add --renormalize .", output)
