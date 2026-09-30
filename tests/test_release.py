"""发布记录的核对（tools/check_release.py）和包自身的版本信息。
Release record checking (tools/check_release.py) and the package's own version metadata.
"""

import contextlib
import importlib.util
import io
import json
import re
import unittest
from pathlib import Path

from fixtures import TempDirTestCase, TestCase, run_git

import xrobot

REPOSITORY = Path(__file__).resolve().parents[1]
_spec = importlib.util.spec_from_file_location(
    "release_checker", REPOSITORY / "tools/check_release.py"
)
release = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(release)


def pyproject(name, version, dependency):
    """夹具包的 pyproject.toml 内容。
    pyproject.toml text for the fixture packages.
    """
    return f"""[build-system]
requires = ["setuptools"]

[project]
name = "{name}"
version = "{version}"
dependencies = [
    "pyyaml",
    "{dependency}",
]

[project.urls]
Homepage = "https://example.invalid"
"""


class PackageMetadata(TestCase):
    """包的版本和对 xr-syntax 的锁定。
    The package version and its xr-syntax pin.
    """

    def test_module_version_equals_the_pyproject_version(self):
        self.assertEqual(xrobot.__version__, release.package_version(REPOSITORY))

    def test_xr_syntax_is_pinned_exactly(self):
        version, requirement = release.xr_syntax_pin(REPOSITORY)
        self.assertIsNotNone(version, requirement)


class ReleaseRecord(TempDirTestCase):
    """发布记录与候选仓库的核对。
    Checking a release record against the candidate repositories.
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp
        self.repos = {}
        self.record = {
            "xr-syntax": "0.2.0",
            "repositories": {},
            "checks": dict.fromkeys(release.GATES, "pass"),
        }
        packages = {"xrobot": ("xrobot", "1.0.0"), "codegen": ("libxr", "6.0.0")}
        for name in release.REPOSITORIES:
            path = self.tmp / name
            path.mkdir()
            self.repos[name] = path
            run_git(path, "init", "-q", "-b", "dev")
            if name in packages:
                self.write(path / "pyproject.toml", pyproject(*packages[name], "xr-syntax==0.2.0"))
            else:
                self.write(path / "CMakeLists.txt", "project(xr)\n")
            run_git(path, "add", "-A")
            run_git(path, "commit", "-q", "-m", "candidate")
            row = {"commit": run_git(path, "rev-parse", "HEAD")}
            if name in packages:
                row["version"] = packages[name][1]
            self.record["repositories"][name] = row

    def recommit(self, name, text):
        """提交新的 pyproject.toml，并把记录中的 commit 改为新提交。
        Commit a new pyproject.toml and record the new commit.
        """
        self.write(self.repos[name] / "pyproject.toml", text)
        run_git(self.repos[name], "commit", "-q", "-am", "change")
        self.record["repositories"][name]["commit"] = run_git(self.repos[name], "rev-parse", "HEAD")

    def test_independent_versions_pass_without_tagging(self):
        self.assertEqual(release.check(self.repos, self.record), [])
        for path in self.repos.values():
            self.assertEqual(run_git(path, "tag", "--list"), "")
            self.assertEqual(run_git(path, "status", "--porcelain"), "")

    def test_each_package_version_must_equal_its_record(self):
        self.record["repositories"]["codegen"]["version"] = "1.0.0"
        self.assertEqual(
            release.check(self.repos, self.record),
            ["codegen: pyproject.toml version 6.0.0 differs from the recorded 1.0.0"],
        )

    def test_a_tag_alone_does_not_change_the_package_version(self):
        for path in self.repos.values():
            run_git(path, "tag", "v1.0.1")
        self.record["repositories"]["xrobot"]["version"] = "1.0.1"
        self.assertEqual(
            release.check(self.repos, self.record),
            ["xrobot: pyproject.toml version 1.0.0 differs from the recorded 1.0.1"],
        )

    def test_versions_must_be_release_versions(self):
        self.record["repositories"]["xrobot"]["version"] = "master"
        self.assertIn(
            "xrobot: the record needs a release version such as 1.0.0 or 1.0.0rc1",
            release.check(self.repos, self.record),
        )
        self.recommit("xrobot", pyproject("xrobot", "1.0.0rc1", "xr-syntax==0.2.0"))
        self.record["repositories"]["xrobot"]["version"] = "1.0.0rc1"
        self.assertEqual(release.check(self.repos, self.record), [])

    def test_both_packages_pin_the_recorded_xr_syntax_exactly(self):
        self.recommit("codegen", pyproject("libxr", "6.0.0", "xr-syntax==0.2.1"))
        self.assertEqual(
            release.check(self.repos, self.record),
            ["codegen pins xr-syntax==0.2.1, the record says 0.2.0"],
        )
        for requirement in (
            "xr-syntax>=0.2.0",
            "xr-syntax~=0.2.0",
            "xr-syntax==0.2.*",
            "xr_syntax",
        ):
            with self.subTest(requirement=requirement):
                self.recommit("codegen", pyproject("libxr", "6.0.0", requirement))
                self.assertEqual(
                    release.check(self.repos, self.record),
                    [f"codegen must pin xr-syntax exactly (xr-syntax==X); found {requirement}"],
                )
        self.recommit("codegen", pyproject("libxr", "6.0.0", "XR_Syntax == 0.2.0"))
        self.assertEqual(release.check(self.repos, self.record), [])
        self.recommit("codegen", pyproject("libxr", "6.0.0", "requests"))
        self.assertEqual(
            release.check(self.repos, self.record),
            ["codegen must pin xr-syntax exactly (xr-syntax==X); found no xr-syntax dependency"],
        )

    def test_the_record_names_the_xr_syntax_version(self):
        del self.record["xr-syntax"]
        failures = release.check(self.repos, self.record)
        self.assertIn("the record needs the pinned xr-syntax version", failures)

    def test_commits_must_be_the_clean_heads(self):
        self.record["repositories"]["libxr"]["commit"] = "0" * 40
        self.write(self.repos["xrobot"] / "uncommitted.py", "# work\n")
        failures = release.check(self.repos, self.record)
        self.assertTrue(
            any(f.startswith("libxr: recorded commit 000000000000 is not HEAD") for f in failures),
            failures,
        )
        self.assertIn("xrobot is not a clean committed candidate", failures)
        self.record["repositories"]["libxr"]["commit"] = "abc"
        self.assertIn(
            "libxr: the record needs the full 40-hex commit", release.check(self.repos, self.record)
        )

    def test_an_unreadable_repository_is_named_and_the_others_are_still_checked(self):
        plain = self.tmp / "plain"
        plain.mkdir()
        self.repos["libxr"] = plain
        self.record["repositories"]["codegen"]["version"] = "6.0.1"
        failures = release.check(self.repos, self.record)
        self.assertEqual(len(failures), 2, failures)
        self.assertRegex(failures[0], rf"^libxr \({re.escape(str(plain))}\): fatal: not a git")
        self.assertEqual(
            failures[1], "codegen: pyproject.toml version 6.0.0 differs from the recorded 6.0.1"
        )

    def test_every_gate_must_pass(self):
        self.record["checks"]["bsp"] = "not-run"
        del self.record["checks"]["docs"]
        self.assertEqual(
            release.check(self.repos, self.record),
            ["Candidate acceptance is missing: docs", "Candidate acceptance is missing: bsp"],
        )

    def test_command_line(self):
        record = self.write(self.tmp / "record.json", json.dumps(self.record))
        argv = [
            "--libxr",
            self.repos["libxr"],
            "--xrobot",
            self.repos["xrobot"],
            "--codegen",
            self.repos["codegen"],
            "--record",
            record,
        ]
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            release.main([str(a) for a in argv])
        self.assertIn("no tag or publication performed", out.getvalue())
        self.record["checks"]["bsp"] = "fail"
        self.write(record, json.dumps(self.record))
        err = io.StringIO()
        with contextlib.redirect_stderr(err), self.assertRaises(SystemExit) as context:
            release.main([str(a) for a in argv])
        self.assertEqual(context.exception.code, 1)
        self.assertEqual(err.getvalue(), "Candidate acceptance is missing: bsp\n")


class ReleaseDocument(TestCase):
    """RELEASE.md 中的记录示例与检查工具一致。
    The record example in RELEASE.md matches the checker.
    """

    def test_the_documented_record_is_what_the_checker_reads(self):
        text = (REPOSITORY / "RELEASE.md").read_text(encoding="utf-8")
        example = json.loads(re.search(r"```json\n(.*?)```", text, re.S).group(1))
        self.assertEqual(set(example["repositories"]), set(release.REPOSITORIES))
        self.assertEqual(tuple(example["checks"]), release.GATES)
        self.assertIn("xr-syntax", example)
        self.assertIn("--libxr ../libxr --xrobot .", text)


if __name__ == "__main__":
    unittest.main()
