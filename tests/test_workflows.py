"""共享模块 CI 工作流（.github/workflows/module-ci.yml）。
The shared Module CI workflow (.github/workflows/module-ci.yml).
"""

import json
import os
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
