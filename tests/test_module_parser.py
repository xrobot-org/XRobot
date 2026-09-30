"""模块 manifest 和锁定模块的发现（xrobot.module_parser）。
Module manifests and the discovery of locked Modules (xrobot.module_parser).
"""

import os

from fixtures import BspTestCase, TestCase, manifest_block, run_git

from xrobot import __version__
from xrobot.module_parser import discover_modules, manifest_from_text, select_module


class LockedDiscovery(BspTestCase):
    """只读取锁定在各自 commit 上的模块。
    Only the Modules locked at their commits are read.
    """

    def test_only_locked_modules_at_their_commits_are_loaded(self):
        self.module("A", "class A { public: A() {} };")
        self.write("Modules/stale/B/B.hpp", "class B { public: B() {} };\n")
        modules = discover_modules(self.root / "Modules", self.root / "xrobot.lock")
        self.assertEqual(set(modules), {"team/A"})
        self.assertEqual(modules["team/A"]["name"], "A")

    def test_a_lock_is_required(self):
        (self.root / "xrobot.lock").unlink()
        with self.assertRaisesMessage(
            ValueError, "xrobot.lock does not exist; run `xrobot setup` to resolve the Modules"
        ):
            discover_modules(self.root / "Modules", self.root / "xrobot.lock")

    def test_every_lock_problem_is_reported_with_its_fix(self):
        a = self.module("A", "class A { public: A() {} };")
        self.module("B", "class B { public: B() {} };")
        self.module("C", "class C { public: C() {} };")
        self.locked["team/A"] = "0" * 40
        self.locked["team/Gone"] = "1" * 40
        self.write_lock()
        import shutil

        shutil.rmtree(
            self.root / "Modules/team/C/.git", onerror=lambda f, p, e: (os.chmod(p, 0o700), f(p))
        )
        with self.assertRaises(ValueError) as context:
            discover_modules(self.root / "Modules", self.root / "xrobot.lock")
        message = str(context.exception)
        self.assertIn(
            f"team/A is checked out at {run_git(a, 'rev-parse', 'HEAD')[:12]} but xrobot.lock pins 000000000000",
            message,
        )
        self.assertIn(
            "Modules/team/C is not a Git checkout; delete it and run `xrobot setup`", message
        )
        self.assertIn("team/Gone from xrobot.lock is not checked out; run `xrobot setup`", message)
        self.assertNotIn("team/B", message)

    def test_a_lock_entry_without_a_commit_is_rejected(self):
        self.module("A", "class A { public: A() {} };")
        self.write("xrobot.lock", "version: 1\nmodules:\n  team/A: {repo: x}\n")
        with self.assertRaisesMessage(ValueError, "xrobot.lock has no commit for team/A"):
            discover_modules(self.root / "Modules", self.root / "xrobot.lock")

    def test_a_lock_entry_leaving_the_modules_directory_is_rejected(self):
        self.write(
            "xrobot.lock", f'version: 1\nmodules:\n  ../../outside: {{commit: "{"0" * 40}"}}\n'
        )
        with self.assertRaisesMessage(ValueError, "Module path leaves directory: ../../outside"):
            discover_modules(self.root / "Modules", self.root / "xrobot.lock")

    def test_module_selection_by_short_name_or_id(self):
        self.module("A", "class A { public: A() {} };")
        self.module("A", "class A { public: A() {} };", owner="other")
        modules = discover_modules(self.root / "Modules", self.root / "xrobot.lock")
        self.assertEqual(select_module(modules, "team/a")["id"], "team/A")
        with self.assertRaisesMessage(ValueError, "Ambiguous Module a; specify other/A, team/A"):
            select_module(modules, "a")
        with self.assertRaisesMessage(ValueError, "Ambiguous Module A; specify other/A, team/A"):
            select_module(modules, "A")
        with self.assertRaisesMessage(ValueError, "Module not found: B"):
            select_module(modules, "B")


class Manifests(TestCase):
    """MODULE MANIFEST V2 的读取。
    Reading MODULE MANIFEST V2.
    """

    def test_allowed_keys(self):
        manifest = manifest_from_text(
            manifest_block("d", ["team/B@dev"], standalone=False), "A.hpp"
        )
        self.assertEqual(
            (manifest.description, manifest.depends, manifest.standalone),
            ("d", ["team/B@dev"], False),
        )

    def test_other_manifests_are_rejected_with_the_fix(self):
        def block(body, version=" V2"):
            return f"/* === MODULE MANIFEST{version} ===\n{body}\n=== END MANIFEST === */"

        keys = "module_description, depends, standalone"
        cases = (
            ("class A {};", "A.hpp: no MODULE MANIFEST V2 block"),
            (
                manifest_block() + manifest_block(),
                "A.hpp: multiple package manifests",
            ),
            (
                block("module_description: old", version=""),
                "A.hpp: this MODULE MANIFEST predates XRobot 1.0; update the Module to MODULE "
                f"MANIFEST V2 with {keys} (the C++ constructor is the interface)",
            ),
            (
                block("module_description: new", version=" V3"),
                f"A.hpp: MODULE MANIFEST V3 needs a newer xrobot; xrobot {__version__} reads "
                "manifests up to V2",
            ),
            (block("- a list"), "A.hpp: package manifest must be a mapping"),
            (
                # 1.0 之前的 manifest 键。
                # Manifest keys from before 1.0.
                block("description: old\nconstructor_args: []\ntemplate_args: []"),
                "A.hpp: unsupported manifest key(s) description, constructor_args, template_args; "
                f"MODULE MANIFEST V2 holds only {keys}",
            ),
            (block("depends: team/B"), "A.hpp: depends must be a list"),
        )
        for text, message in cases:
            with self.subTest(message=message):
                with self.assertRaises(ValueError) as context:
                    manifest_from_text(text, "A.hpp")
                self.assertEqual(str(context.exception), message)
