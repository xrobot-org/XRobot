"""共享工作流：模块 CI（.github/workflows/module-ci.yml）和 STM32 BSP CI
（.github/workflows/bsp-stm32-ci.yml）。
The shared workflows: Module CI (.github/workflows/module-ci.yml) and STM32 BSP CI
(.github/workflows/bsp-stm32-ci.yml).
"""

import contextlib
import hashlib
import io
import json
import os
import shutil
import subprocess
import tarfile
import unittest
from pathlib import Path
from unittest import mock

import yaml
from fixtures import CliMixin, TempDirTestCase, UpstreamTestCase, run_git

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
                        "modules": [
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
                return Index({"modules": [{"id": "team/B", "type": "module", "repo": b.as_uri()}]})
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

    def plan(self, configs, presets="", release_configs=""):
        """用这些输入运行 plan 作业的脚本，返回输出的构建列表和发布列表。
        Run the script of the plan job with these inputs and return the builds and releases.
        """
        step = next(s for s in self.shared["jobs"]["plan"]["steps"] if s.get("id") == "plan")
        script = embedded_script(step, "PYCODE")
        output = self.tmp / "github_output"
        output.write_text("", encoding="utf-8")
        environment = {
            "XR_CONFIGS": configs,
            "XR_PRESETS": presets,
            "XR_RELEASE_CONFIGS": release_configs,
            "GITHUB_OUTPUT": str(output),
        }
        with mock.patch.dict(os.environ, environment):
            exec(compile(script, "<plan>", "exec"), {"__name__": "__ci__"})
        values = dict(
            line.split("=", 1) for line in output.read_text(encoding="utf-8").split("\n") if line
        )
        return json.loads(values["builds"]), json.loads(values["releases"])

    def choose_tag(self, event_tag="", tags=(), sha="c" * 40):
        """用这些 tag 运行 release 作业选 tag 的脚本，返回 (tag, publish)。
        Run the tag choice script of the release job with these tags and return
        (tag, publish).
        """
        step = next(s for s in self.shared["jobs"]["release"]["steps"] if s.get("id") == "tag")
        script = embedded_script(step, "TAGCODE")
        env_file, output = self.tmp / "github_env", self.tmp / "github_output"
        env_file.write_text("", encoding="utf-8")
        output.write_text("", encoding="utf-8")
        listing = "".join(f"{name}\t{commit}\n" for name, commit in tags)
        environment = {
            "XR_EVENT_TAG": event_tag,
            "GITHUB_REPOSITORY": "team/bsp",
            "GITHUB_SHA": sha,
            "GITHUB_ENV": str(env_file),
            "GITHUB_OUTPUT": str(output),
        }
        listed = subprocess.CompletedProcess([], 0, stdout=listing, stderr="")
        with mock.patch.dict(os.environ, environment), mock.patch("subprocess.run", return_value=listed):
            with contextlib.redirect_stdout(io.StringIO()):
                exec(compile(script, "<tag>", "exec"), {"__name__": "__ci__"})
        tag = env_file.read_text(encoding="utf-8").strip().removeprefix("XR_TAG=")
        publish = output.read_text(encoding="utf-8").strip().removeprefix("publish=")
        return tag, publish

    def test_a_push_to_master_publishes_the_next_patch_tag(self):
        # 以前只有 tag 推送才发布固件，1.0 发布时没有人给 BSP 打 tag，一个固件也没有发布。
        # Only a tag push used to publish firmware; nobody tagged the BSPs at the 1.0 release,
        # so no firmware was published.
        release = self.shared["jobs"]["release"]["if"]
        self.assertIn("github.ref == 'refs/heads/master'", release)
        self.assertEqual(self.choose_tag(), ("v1.0.0", "true"))
        tags = [("v1.0.0", "a" * 40), ("v1.2.3", "b" * 40), ("v1.10.0", "d" * 40), ("V9.0.0", "e" * 40)]
        self.assertEqual(self.choose_tag(tags=tags), ("v1.10.1", "true"))
        # 合并提交已有 v tag 时由那个 tag 的运行发布；tag 推送和 Release 发布各自的 tag。
        # A merge commit that already has a v tag is published by the run of that tag; a tag
        # push and a Release publish their own tag.
        self.assertEqual(self.choose_tag(tags=tags, sha="b" * 40)[1], "false")
        self.assertEqual(self.choose_tag(event_tag="v2.0.0", tags=tags), ("v2.0.0", "true"))

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
        self.assertEqual(inputs["release-configs"]["default"], "")
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
        # 检查只跑一次；每个构建解析锁定的模块，配置，构建，打包并上传；发布作业汇总全部构建。
        # The checks run once; every build resolves the locked Modules, configures, builds,
        # packages and uploads; the release job gathers all builds.
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
        self.assertEqual(
            self.steps["release"],
            [
                "Choose the tag",
                "Download firmware artifacts",
                "Assemble the release files",
                "Keep the description of an existing release",
                "Publish the release files",
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

    def test_the_builds_to_publish_are_packaged_and_one_job_publishes_all_of_them(self):
        build = self.shared["jobs"]["build"]["steps"]
        packaging = [s for s in build if s["name"].startswith(("Package", "Upload"))]
        self.assertEqual([s["if"] for s in packaging], ["matrix.release"] * 2)
        upload = packaging[1]["with"]
        self.assertEqual(upload["path"], "dist/${{ matrix.name }}")
        self.assertEqual(upload["if-no-files-found"], "error")
        release = self.shared["jobs"]["release"]
        # 一个作业下载全部构建产物，一次上传，SHA256SUMS 才完整。
        # One job downloads every build artifact and uploads once, so SHA256SUMS is complete.
        self.assertNotIn("strategy", release)
        self.assertEqual(release["permissions"], {"contents": "write"})
        self.assertEqual(release["needs"], ["plan", "check", "build"])
        self.assertIn("github.event_name == 'release'", release["if"])
        self.assertIn("startsWith(github.ref, 'refs/tags/v')", release["if"])
        self.assertEqual(self.shared["permissions"], {"contents": "read"})
        tag, download, assemble, notes, publish = release["steps"]
        self.assertEqual(tag["id"], "tag")
        for step in (download, assemble, notes, publish):
            self.assertEqual(step["if"], "steps.tag.outputs.publish == 'true'")
        self.assertEqual(publish["with"]["tag_name"], "${{ env.XR_TAG }}")
        self.assertEqual(publish["with"]["target_commitish"], "${{ github.sha }}")
        self.assertEqual(download["with"]["pattern"], "*-firmware")
        self.assertEqual(publish["uses"], "softprops/action-gh-release@v2")
        self.assertEqual(publish["with"]["files"], "release/*")
        self.assertIs(publish["with"]["fail_on_unmatched_files"], True)
        self.assertNotIn("body", publish["with"])
        self.assertEqual(publish["with"]["body_path"], "${{ steps.notes.outputs.path }}")
        self.assertEqual(notes["id"], "notes")

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
                {"config": "default", "preset": "", "name": "default", "release": False},
                {"config": "hero", "preset": "", "name": "hero", "release": True},
                {"config": "sentry", "preset": "", "name": "sentry", "release": True},
            ],
        )
        self.assertEqual([b["name"] for b in releases], ["hero", "sentry"])
        self.assertEqual(
            self.plan("")[0],
            [{"config": "default", "preset": "", "name": "default", "release": True}],
        )

    def test_a_bsp_that_builds_only_default_publishes_default(self):
        for configs in ("default", "", "default\ndefault"):
            builds, releases = self.plan(configs)
            self.assertEqual([b["name"] for b in releases], ["default"], configs)
            self.assertEqual(builds, releases)

    def test_robot_configurations_are_published_and_default_is_not(self):
        builds, releases = self.plan("sentry\ndefault\nhero")
        self.assertEqual([b["name"] for b in builds], ["sentry", "default", "hero"])
        self.assertEqual([b["name"] for b in releases], ["sentry", "hero"])
        self.assertEqual([b["release"] for b in builds], [True, False, True])
        # 没有 default 时，每个配置都发布。
        # Without default, every configuration is published.
        self.assertEqual([b["name"] for b in self.plan("hero\nsentry")[1]], ["hero", "sentry"])

    def test_release_configs_chooses_what_is_published(self):
        builds, releases = self.plan("default\nhero\nsentry", release_configs="\n sentry \n")
        self.assertEqual([b["name"] for b in releases], ["sentry"])
        self.assertEqual([b["release"] for b in builds], [False, False, True])
        # 可以发布 default，也可以与其他配置一起发布。
        # default can be published, alone or with others.
        self.assertEqual(
            [b["name"] for b in self.plan("default\nhero", release_configs="default")[1]],
            ["default"],
        )
        self.assertEqual(
            [b["name"] for b in self.plan("default\nhero", release_configs="hero\ndefault")[1]],
            ["default", "hero"],
        )
        # 空的 release-configs 回到默认规则。
        # An empty release-configs falls back to the default rule.
        self.assertEqual(
            [b["name"] for b in self.plan("default\nhero", release_configs=" \n")[1]], ["hero"]
        )

    def test_release_configs_names_only_configs_that_are_built(self):
        printed = io.StringIO()
        with contextlib.redirect_stdout(printed), self.assertRaises(SystemExit):
            self.plan("default\nhero", release_configs="hero\nsentry")
        self.assertIn("release-configs lists sentry, which is not in configs", printed.getvalue())

    def test_the_plan_crosses_configurations_with_presets(self):
        builds, releases = self.plan("default\nhero", "app\nbootloader")
        self.assertEqual(
            [b["name"] for b in builds],
            ["default-app", "default-bootloader", "hero-app", "hero-bootloader"],
        )
        self.assertEqual([b["preset"] for b in builds], ["app", "bootloader"] * 2)
        self.assertEqual([b["name"] for b in releases], ["hero-app", "hero-bootloader"])
        self.assertEqual([b["name"] for b in self.plan("default", "app")[0]], ["default-app"])

    def test_every_preset_image_of_a_published_configuration_is_published(self):
        # default 单独构建时发布它的每个 preset，release-configs 选中的配置也一样。
        # A lone default publishes each of its presets, and so do the configurations that
        # release-configs chooses.
        releases = self.plan("default", "app\nbootloader")[1]
        self.assertEqual([b["name"] for b in releases], ["default-app", "default-bootloader"])
        releases = self.plan("default\nhero\nsentry", "app\nbootloader", "sentry")[1]
        self.assertEqual([b["name"] for b in releases], ["sentry-app", "sentry-bootloader"])
        self.assertEqual(
            [(b["config"], b["preset"]) for b in releases],
            [("sentry", "app"), ("sentry", "bootloader")],
        )

    def test_the_plan_refuses_names_that_are_not_plain(self):
        for configs, presets, chosen in (
            ("a b", "", ""),
            ("hero;rm", "", ""),
            ("$(id)", "", ""),
            ("default", "x/y", ""),
            ("default\nhero", "", "he ro"),
        ):
            printed = io.StringIO()
            with (
                self.subTest(configs=configs, presets=presets, chosen=chosen),
                contextlib.redirect_stdout(printed),
                self.assertRaises(SystemExit),
            ):
                self.plan(configs, presets, chosen)
            self.assertIn("is not a plain name", printed.getvalue())

    def test_the_plan_refuses_configurations_and_presets_with_the_same_build_name(self):
        printed = io.StringIO()
        with contextlib.redirect_stdout(printed), self.assertRaises(SystemExit):
            self.plan("a-b\na", "c\nb-c")
        self.assertIn("give the same build name: a-b-c", printed.getvalue())

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


LIBXR_COMMIT = "6c51bf4084d983bc20309e63818ce104dc53f959"
MODULE_COMMIT = "44424519645a0d9297d5a7bd57b8dd4e0c342e74"
SIZE_OUTPUT = (
    "   text\t   data\t    bss\t    dec\t    hex\tfilename\n"
    " 337416\t   2160\t  54212\t 393788\t  60a3c\tbuild/DevC.elf\n"
)
TOOLS = {
    "xrobot": {"version": "1.0.0", "pin": "1.0.0"},
    "libxr": {"version": "6.0.0", "pin": "6.0.0"},
}


class ReleaseJob(TempDirTestCase):
    """发布路径上的脚本：打包步骤记录构建信息，发布作业生成发布文件、校验和、清单和说明。
    The scripts on the release path: the package step records the build information, and the
    release job writes the release files, the checksums, the manifest and the notes.
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp
        self.shared = load_bsp_workflow()

    def script(self, job, step, marker):
        """作业中这个步骤里嵌入的脚本。
        The script embedded in this step of a job.
        """
        found = next(s for s in self.shared["jobs"][job]["steps"] if s.get("name") == step)
        return embedded_script(found, marker)

    def run_script(self, script, folder, **environment):
        """在 folder 中以这些环境变量运行脚本，返回它打印的内容（也保存在 self.printed）。
        Run the script in folder with these environment variables and return what it prints
        (kept in self.printed as well).
        """
        self.printed = io.StringIO()
        previous = os.getcwd()
        os.chdir(str(folder))
        try:
            with (
                mock.patch.dict(os.environ, environment),
                contextlib.redirect_stdout(self.printed),
            ):
                exec(compile(script, "<release>", "exec"), {"__name__": "__ci__"})
        finally:
            os.chdir(previous)
        return self.printed.getvalue()

    def refused(self, action, message):
        """运行 action，它以 SystemExit 失败，并且打印了 message。
        Run action, which fails with SystemExit and has printed message.
        """
        with self.assertRaises(SystemExit):
            action()
        self.assertIn(message, self.printed.getvalue())

    def bsp(self):
        """已提交 xrobot.lock、两个固定版本和 LibXR 子模块记录的 BSP 仓库。
        A BSP repository with a committed xrobot.lock, the two pins and a recorded LibXR
        submodule.
        """
        repo = self.tmp / "bsp"
        run_git(None, "init", "-q", str(repo))
        lock = {
            "version": 1,
            "modules": {
                "xrobot-org/BuzzerAlarm": {"commit": MODULE_COMMIT, "resolved_ref": "dev"},
                "team/A": {"commit": "a" * 40},
            },
        }
        self.write(repo / "xrobot.lock", yaml.safe_dump(lock))
        self.write(repo / "Modules/modules.yaml", "xrobot: 1.0.0\nmodules: []\n")
        self.write(repo / "User/libxr_config.yaml", "generator: 6.0.0\n")
        run_git(repo, "add", ".")
        # 子模块记录是没有检出的 gitlink；git add . 会把它当作已删除，所以放在最后。
        # The submodule record is a gitlink that is not checked out; git add . would stage it
        # as deleted, so it goes in last.
        gitlink = f"160000,{LIBXR_COMMIT},Drivers/LibXR"
        run_git(repo, "update-index", "--add", "--cacheinfo", gitlink)
        run_git(repo, "commit", "-q", "-m", "BSP")
        return repo

    def package(self, repo, name="hero", config="hero", preset="", size=SIZE_OUTPUT):
        """在 BSP 上运行打包步骤中记录构建信息的脚本，返回写出的 build-info.json 内容。
        Run the script of the package step that records the build information in the BSP and
        return the build-info.json it writes.
        """
        folder = repo / "dist" / name
        for extension in ("elf", "hex", "bin"):
            self.write(folder / f"DevC-{name}.{extension}", f"{name} {extension}")
        self.write(folder / f"DevC-{config}.yaml", f"config: {config}\n")
        script = self.script("build", "Package firmware", "INFOCODE")
        versions = {"xrobot": "1.0.0", "libxr": "6.0.0"}
        with mock.patch("importlib.metadata.version", side_effect=versions.__getitem__):
            self.run_script(
                script,
                repo,
                XR_PROJECT="DevC",
                XR_NAME=name,
                XR_CONFIG=config,
                XR_PRESET=preset,
                XR_SIZE_OUTPUT=size,
            )
        return json.loads(self.read(folder / "build-info.json"))

    def test_the_package_step_records_what_the_build_used(self):
        repo = self.bsp()
        info = self.package(repo)
        self.assertEqual(
            info["files"],
            {
                "elf": "DevC-hero.elf",
                "hex": "DevC-hero.hex",
                "bin": "DevC-hero.bin",
                "config": "DevC-hero.yaml",
            },
        )
        self.assertEqual((info["name"], info["config"], info["preset"]), ("hero", "hero", ""))
        self.assertEqual(info["size"], {"text": 337416, "data": 2160, "bss": 54212})
        self.assertEqual(info["commit"], run_git(repo, "rev-parse", "HEAD"))
        self.assertEqual(info["tools"], TOOLS)
        self.assertEqual(info["libxr_submodule"], {"path": "Drivers/LibXR", "commit": LIBXR_COMMIT})
        self.assertEqual(
            info["modules"], {"team/A": "a" * 40, "xrobot-org/BuzzerAlarm": MODULE_COMMIT}
        )

    def test_the_package_step_records_a_preset_and_a_bsp_without_the_libxr_submodule(self):
        repo = self.bsp()
        run_git(repo, "rm", "-q", "--cached", "Drivers/LibXR")
        run_git(repo, "commit", "-q", "-m", "No submodule")
        info = self.package(repo, "default-app", "default", "app")
        self.assertEqual((info["config"], info["preset"]), ("default", "app"))
        self.assertEqual(info["files"]["elf"], "DevC-default-app.elf")
        self.assertEqual(info["files"]["config"], "DevC-default.yaml")
        self.assertIsNone(info["libxr_submodule"])

    def test_the_package_step_refuses_what_it_cannot_read(self):
        repo = self.bsp()
        self.refused(
            lambda: self.package(repo, size="no sizes here"), "cannot read the section sizes"
        )

    def fragment(self, config, preset="", commit=None, config_text=None):
        """一个构建的构建产物文件夹，内容与打包步骤上传的一致；返回它的 build-info。
        The artifact folder of a build, laid out as the package step uploads it; return its
        build-info.
        """
        name = f"{config}-{preset}" if preset else config
        folder = self.tmp / "release-artifacts" / f"{name}-firmware"
        files = {
            "elf": f"DevC-{name}.elf",
            "hex": f"DevC-{name}.hex",
            "bin": f"DevC-{name}.bin",
            "config": f"DevC-{config}.yaml",
        }
        for kind, file in files.items():
            if kind == "config":
                self.write(folder / file, config_text or f"config: {config}\n")
            else:
                self.write(folder / file, f"{name} {kind}")
        info = {
            "name": name,
            "config": config,
            "preset": preset,
            "files": files,
            "size": {"text": 1000 + len(name), "data": 20, "bss": 300},
            "commit": commit or "0123456789abcdef0123456789abcdef01234567",
            "tools": TOOLS,
            "libxr_submodule": {"path": "Drivers/LibXR", "commit": LIBXR_COMMIT},
            "modules": {"xrobot-org/BuzzerAlarm": MODULE_COMMIT},
        }
        self.write(folder / "build-info.json", json.dumps(info))
        return {"config": config, "preset": preset, "name": name, "release": True}

    def assemble(self, releases, tag="v1.2.0", **environment):
        """用这些构建运行发布作业中生成发布文件的脚本，返回它打印的内容。
        Run the script of the release job that writes the release files for these builds and
        return what it prints.
        """
        values = {
            "XR_PROJECT": "DevC",
            "XR_TAG": tag,
            "XR_REPOSITORY": "xrobot-org/bsp-dev-c",
            "XR_RELEASES": json.dumps(releases),
            "XR_TOOLCHAIN": "cmake/starm-clang.cmake",
            "XR_BUILD_TYPE": "Release",
            "XR_IMAGE": "ghcr.io/xrobot-org/docker-image-stm32:main",
        }
        values.update(environment)
        script = self.script("release", "Assemble the release files", "RELEASECODE")
        return self.run_script(script, self.tmp, **values)

    def test_the_release_gets_named_files_checksums_a_manifest_and_notes(self):
        releases = [self.fragment("hero"), self.fragment("sentry")]
        printed = self.assemble(releases)
        release = self.tmp / "release"
        names = sorted(path.name for path in release.iterdir())
        self.assertEqual(
            names,
            sorted(
                [
                    f"DevC-{config}-v1.2.0.{ext}"
                    for config in ("hero", "sentry")
                    for ext in ("elf", "hex", "bin", "yaml", "tar.gz")
                ]
                + ["SHA256SUMS", "firmware-manifest.json"]
            ),
        )
        self.assertEqual((release / "DevC-hero-v1.2.0.elf").read_bytes(), b"hero elf")
        self.assertEqual((release / "DevC-hero-v1.2.0.yaml").read_bytes(), b"config: hero\n")
        # SHA256SUMS 列出每个上传的文件（不含自己），格式可用 sha256sum -c 校验。
        # SHA256SUMS lists every uploaded file but itself, in the format sha256sum -c reads.
        sums = (release / "SHA256SUMS").read_bytes().decode("utf-8")
        expected = "".join(
            hashlib.sha256((release / name).read_bytes()).hexdigest() + f"  {name}\n"
            for name in names
            if name != "SHA256SUMS"
        )
        self.assertEqual(sums, expected)
        self.assertEqual(printed, sums)
        self.assertIn("firmware-manifest.json", sums)
        self.assertNotIn("SHA256SUMS\n", sums)
        # 每份配置一个归档，内容是这份配置的文件。
        # One archive per configuration, holding the files of that configuration.
        with tarfile.open(release / "DevC-sentry-v1.2.0.tar.gz") as archive:
            self.assertEqual(
                archive.getnames(),
                [
                    f"DevC-sentry-v1.2.0/DevC-sentry-v1.2.0.{ext}"
                    for ext in ("bin", "elf", "hex", "yaml")
                ],
            )
            member = archive.extractfile("DevC-sentry-v1.2.0/DevC-sentry-v1.2.0.hex")
            self.assertEqual(member.read(), b"sentry hex")
        # 同样的输入得到同样的归档。
        # The same inputs give the same archive.
        first = (release / "DevC-hero-v1.2.0.tar.gz").read_bytes()
        self.assemble(releases)
        self.assertEqual((release / "DevC-hero-v1.2.0.tar.gz").read_bytes(), first)

    def test_the_manifest_records_the_build_and_what_it_was_built_from(self):
        self.assemble([self.fragment("hero"), self.fragment("sentry")])
        manifest = json.loads(self.read(self.tmp / "release/firmware-manifest.json"))
        self.assertEqual(
            {key: value for key, value in manifest.items() if key != "builds"},
            {
                "schema": 1,
                "tag": "v1.2.0",
                "commit": "0123456789abcdef0123456789abcdef01234567",
                "repository": "xrobot-org/bsp-dev-c",
                "project": "DevC",
                "build_type": "Release",
                "toolchain": "cmake/starm-clang.cmake",
                "image": "ghcr.io/xrobot-org/docker-image-stm32:main",
                "tools": TOOLS,
                "libxr_submodule": {"path": "Drivers/LibXR", "commit": LIBXR_COMMIT},
                "modules": {"xrobot-org/BuzzerAlarm": MODULE_COMMIT},
            },
        )
        self.assertEqual(
            manifest["builds"][1],
            {
                "config": "sentry",
                "preset": None,
                "files": {
                    "elf": "DevC-sentry-v1.2.0.elf",
                    "hex": "DevC-sentry-v1.2.0.hex",
                    "bin": "DevC-sentry-v1.2.0.bin",
                    "config": "DevC-sentry-v1.2.0.yaml",
                    "archive": "DevC-sentry-v1.2.0.tar.gz",
                },
                "size": {"text": 1006, "data": 20, "bss": 300},
            },
        )
        self.assertEqual([b["config"] for b in manifest["builds"]], ["hero", "sentry"])

    def test_presets_give_every_image_its_own_files_and_share_the_configuration(self):
        releases = [self.fragment("hero", "app"), self.fragment("hero", "bootloader")]
        self.assemble(releases, tag="v2.0.0-rc.1")
        release = self.tmp / "release"
        self.assertEqual(
            sorted(path.name for path in release.iterdir()),
            sorted(
                [
                    f"DevC-hero-{image}-v2.0.0-rc.1.{ext}"
                    for image in ("app", "bootloader")
                    for ext in ("elf", "hex", "bin")
                ]
                + ["DevC-hero-v2.0.0-rc.1.yaml", "DevC-hero-v2.0.0-rc.1.tar.gz"]
                + ["SHA256SUMS", "firmware-manifest.json"]
            ),
        )
        with tarfile.open(release / "DevC-hero-v2.0.0-rc.1.tar.gz") as archive:
            self.assertEqual(len(archive.getnames()), 7)
        manifest = json.loads(self.read(release / "firmware-manifest.json"))
        self.assertEqual([b["preset"] for b in manifest["builds"]], ["app", "bootloader"])
        # 有 preset 时，工具链和构建类型由 preset 决定。
        # With presets, the presets choose the toolchain and the build type.
        self.assertIsNone(manifest["build_type"])
        self.assertIsNone(manifest["toolchain"])
        notes = self.read(self.tmp / "release-notes.md")
        self.assertIn(
            "| `hero` | `bootloader` | 1015 | 20 | 300 | `DevC-hero-v2.0.0-rc.1.tar.gz` |", notes
        )
        self.assertIn("- Image `ghcr.io/xrobot-org/docker-image-stm32:main`\n", notes)
        self.assertEqual(manifest["builds"][0]["files"]["elf"], "DevC-hero-app-v2.0.0-rc.1.elf")

    def test_characters_a_file_name_cannot_have_leave_the_tag_in_the_file_names_only(self):
        self.assemble([self.fragment("hero")], tag="v1.0/beta+1")
        self.assertTrue((self.tmp / "release/DevC-hero-v1.0-beta-1.elf").is_file())
        manifest = json.loads(self.read(self.tmp / "release/firmware-manifest.json"))
        self.assertEqual(manifest["tag"], "v1.0/beta+1")

    def test_the_notes_list_the_builds_with_sizes_and_the_tool_versions(self):
        self.assemble([self.fragment("hero"), self.fragment("sentry")])
        notes = self.read(self.tmp / "release-notes.md")
        self.assertTrue(notes.startswith("## DevC v1.2.0\n"))
        self.assertIn("| Config | Preset | text | data | bss | Archive |", notes)
        self.assertIn("| `hero` | - | 1004 | 20 | 300 | `DevC-hero-v1.2.0.tar.gz` |", notes)
        self.assertIn("| `sentry` | - | 1006 | 20 | 300 | `DevC-sentry-v1.2.0.tar.gz` |", notes)
        self.assertIn("xrobot 1.0.0, libxr 6.0.0, LibXR `6c51bf4`", notes)
        self.assertIn("Commit `0123456` of xrobot-org/bsp-dev-c", notes)
        self.assertIn(
            "- Image `ghcr.io/xrobot-org/docker-image-stm32:main`, toolchain "
            "`cmake/starm-clang.cmake`, build type `Release`\n",
            notes,
        )
        self.assertIn("`SHA256SUMS`", notes)
        self.assertNotIn("\r", notes)
        # 说明不在上传的文件里。
        # The notes are not among the uploaded files.
        self.assertFalse((self.tmp / "release/release-notes.md").exists())

    def test_a_missing_or_inconsistent_build_stops_the_release(self):
        hero = self.fragment("hero")
        sentry = self.fragment("sentry", commit="f" * 40)
        absent = {"config": "dart", "preset": "", "name": "dart", "release": True}
        cases = (
            ([hero, absent], "the artifact dart-firmware is missing"),
            ([hero, sentry], "the builds hero and sentry differ in commit"),
            ([{**hero, "config": "dart"}], "the artifact hero-firmware belongs to another build"),
        )
        for releases, message in cases:
            with self.subTest(message=message):
                self.refused(lambda releases=releases: self.assemble(releases), message)

    def test_the_builds_of_a_configuration_must_agree_on_its_configuration_file(self):
        app = self.fragment("hero", "app")
        bootloader = self.fragment("hero", "bootloader", config_text="other: 1\n")
        self.refused(
            lambda: self.assemble([app, bootloader]),
            "the builds of hero use different configuration files",
        )

    def test_a_missing_file_in_an_artifact_stops_the_release(self):
        hero = self.fragment("hero")
        (self.tmp / "release-artifacts/hero-firmware/DevC-hero.hex").unlink()
        self.refused(
            lambda: self.assemble([hero]), "the artifact hero-firmware has no DevC-hero.hex"
        )

    def notes_path(self, event, found):
        """用 gh 返回 found 运行"保留已有发布说明"步骤，返回它输出的 path。
        Run the step that keeps an existing release description with gh returning found and
        return the path it outputs.
        """
        script = self.script("release", "Keep the description of an existing release", "NOTESCODE")
        output = self.tmp / "github_output"
        output.write_text("", encoding="utf-8")
        with mock.patch("subprocess.run", return_value=found) as run:
            self.run_script(
                script,
                self.tmp,
                GITHUB_EVENT_NAME=event,
                GITHUB_REPOSITORY="xrobot-org/bsp-dev-c",
                GITHUB_OUTPUT=str(output),
                XR_TAG="v1.2.0",
            )
        self.requested = run.call_args
        return output.read_text(encoding="utf-8").strip()

    def test_a_tag_push_writes_the_description_unless_the_release_already_has_one(self):
        def found(code, out="", error=""):
            return subprocess.CompletedProcess([], code, out, error)

        self.assertEqual(
            self.notes_path("push", found(1, "", "gh: Not Found (HTTP 404)")),
            "path=release-notes.md",
        )
        self.assertIn("repos/xrobot-org/bsp-dev-c/releases/tags/v1.2.0", self.requested.args[0][2])
        self.assertEqual(self.notes_path("push", found(0, "\n")), "path=release-notes.md")
        self.assertEqual(self.notes_path("push", found(0, "Written by a person\n")), "path=")

    def test_a_published_release_keeps_its_description(self):
        script = self.script("release", "Keep the description of an existing release", "NOTESCODE")
        output = self.tmp / "github_output"
        output.write_text("", encoding="utf-8")
        with mock.patch("subprocess.run") as run:
            self.run_script(
                script,
                self.tmp,
                GITHUB_EVENT_NAME="release",
                GITHUB_REPOSITORY="xrobot-org/bsp-dev-c",
                GITHUB_OUTPUT=str(output),
                XR_TAG="v1.2.0",
            )
        run.assert_not_called()
        self.assertEqual(output.read_text(encoding="utf-8").strip(), "path=")

    def test_a_release_that_cannot_be_read_stops_the_job(self):
        failure = subprocess.CompletedProcess([], 1, "", "gh: Bad credentials (HTTP 401)")
        self.refused(lambda: self.notes_path("push", failure), "Cannot read the release of the tag")
