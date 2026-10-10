"""解析模块请求和 xrobot.lock（xrobot.lock）。
Resolving Module requests and xrobot.lock (xrobot.lock).
"""

import contextlib
import io
import unittest
from unittest import mock

import requests
import yaml
from fixtures import TestCase, UpstreamTestCase, run_git

from xrobot.lock import read_modules_yaml, repository_identity, same_repository, write_cmake
from xrobot.module_parser import discover_modules
from xrobot.source_manager import (
    SourceManager,
    SourceUnavailable,
)


class Resolution(UpstreamTestCase):
    """把请求解析为 commit，写入 xrobot.lock。
    Resolving requests to commits written to xrobot.lock.
    """

    def test_dependencies_resolve_to_exact_commits_and_a_module_list(self):
        b = self.upstream("team/B")
        self.upstream("team/A", ["team/B@master"])
        self.configure(["team/A@master"])
        lock = self.sync()
        self.assertEqual(set(lock["modules"]), {"team/A", "team/B"})
        self.assertEqual(lock["version"], 1)
        self.assertEqual(lock["requests"], [{"id": "team/A", "ref": "master"}])
        self.assertEqual(lock["modules"]["team/B"]["commit"], run_git(b, "rev-parse", "HEAD"))
        self.assertEqual(self.head("team/B"), lock["modules"]["team/B"]["commit"])
        self.assertEqual(yaml.safe_load(self.lock_bytes()), lock)
        cmake = (self.modules / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertIn('include("${CMAKE_CURRENT_LIST_DIR}/team/A/CMakeLists.txt")', cmake)
        self.assertIn('include("${CMAKE_CURRENT_LIST_DIR}/team/B/CMakeLists.txt")', cmake)
        self.assertFalse((self.modules / "team/A/xrobot.lock").exists())

    def test_module_checkouts_keep_the_repository_line_endings(self):
        self.upstream("team/A")
        self.configure(["team/A"])
        self.sync()
        folder = self.modules / "team/A"
        self.assertEqual(run_git(folder, "config", "--local", "core.autocrlf"), "false")

    def test_the_default_branch_is_used_without_a_ref(self):
        self.upstream("team/A")
        self.configure(["team/A"])
        row = self.sync()["modules"]["team/A"]
        self.assertEqual((row["ref_kind"], row["resolved_ref"]), ("branch", "master"))

    def test_the_lock_is_stable_and_updates_are_explicit(self):
        a = self.upstream("team/A")
        self.configure(["team/A@master"])
        old = self.sync()["modules"]["team/A"]["commit"]
        new = self.commit(a, [], "change")
        self.assertEqual(self.sync()["modules"]["team/A"]["commit"], old)
        self.assertEqual(self.sync(frozen=True)["modules"]["team/A"]["commit"], old)
        self.assertEqual(self.sync(update=[])["modules"]["team/A"]["commit"], new)
        self.assertEqual(self.head("team/A"), new)

    def test_frozen_restores_the_lock_without_writing_it(self):
        a = self.upstream("team/A")
        self.configure(["team/A"])
        locked = self.sync()["modules"]["team/A"]["commit"]
        self.commit(a, [], "change")
        path = self.root / "xrobot.lock"
        path.write_bytes(b"# reviewed\n" + self.lock_bytes())
        before = path.read_bytes()
        run_git(self.modules / "team/A", "fetch", "-q", "origin")
        run_git(self.modules / "team/A", "checkout", "-q", "--detach", "origin/master")
        self.assertEqual(self.sync(frozen=True)["modules"]["team/A"]["commit"], locked)
        self.assertEqual(self.head("team/A"), locked)
        self.assertEqual(path.read_bytes(), before)

    def test_frozen_requires_a_lock_that_matches_modules_yaml(self):
        self.upstream("team/A")
        self.upstream("team/C")
        self.configure(["team/A"])
        with self.assertRaisesMessage(
            ValueError,
            "xrobot.lock does not exist; run `xrobot setup` once without --frozen or --offline",
        ):
            self.sync(frozen=True)
        self.sync()
        before = self.lock_bytes()
        self.configure(["team/A", "team/C"])
        with self.assertRaisesMessage(
            ValueError,
            "Modules/modules.yaml differs from xrobot.lock (+team/C); run `xrobot setup` to "
            "update the lock",
        ):
            self.sync(frozen=True)
        self.configure(["team/A@dev"])
        with self.assertRaisesMessage(
            ValueError,
            "Modules/modules.yaml differs from xrobot.lock (~team/A); run `xrobot setup` to "
            "update the lock",
        ):
            self.sync(frozen=True)
        self.configure(["team/C"])
        with self.assertRaisesMessage(
            ValueError,
            "Modules/modules.yaml differs from xrobot.lock (+team/C, -team/A); run `xrobot "
            "setup` to update the lock",
        ):
            self.sync(frozen=True)
        self.assertEqual(self.lock_bytes(), before)

    def test_request_order_and_id_case_do_not_change_the_lock(self):
        self.upstream("team/A")
        self.upstream("team/B")
        self.configure(["team/B@dev", "team/A@dev"])
        first = self.sync()
        self.assertEqual([r["id"] for r in first["requests"]], ["team/A", "team/B"])
        self.configure(["team/a@dev", "team/B@dev"])
        self.assertEqual(self.sync(frozen=True, offline=True)["modules"], first["modules"])
        self.configure(["team/A@master", "team/B@dev"])
        with self.assertRaisesMessage(
            ValueError,
            "Modules/modules.yaml differs from xrobot.lock (~team/A); run `xrobot setup` to "
            "update the lock",
        ):
            self.sync(frozen=True)

    def test_update_cannot_be_combined_with_frozen_or_offline(self):
        self.configure([])
        for flags in ({"frozen": True}, {"offline": True}):
            with (
                self.subTest(flags=flags),
                self.assertRaisesMessage(
                    ValueError, "--update cannot be combined with --frozen or --offline"
                ),
            ):
                self.sync(update=[], **flags)

    def test_offline_needs_neither_sources_nor_remote(self):
        a = self.upstream("team/A")
        self.configure(["team/A"])
        old = self.sync()["modules"]["team/A"]["commit"]
        a.rename(a.with_name("unavailable"))
        # lock 满足且检出里有锁定的 commit 时，不需要远端。
        # An up-to-date lock whose commits are checked out needs no remote.
        self.assertEqual(self.sync()["modules"]["team/A"]["commit"], old)
        with self.assertRaisesRegex(ValueError, r"fetch(.|\n)*`xrobot setup --offline` works"):
            self.sync(update=[])
        self.index.unlink()
        (self.modules / "sources.yaml").unlink()
        self.assertEqual(self.sync(offline=True)["modules"]["team/A"]["commit"], old)

    def test_an_up_to_date_lock_does_not_read_the_sources(self):
        self.upstream("team/A")
        self.configure(["team/A"])
        first = self.sync()
        self.write_yaml(self.modules / "sources.yaml", {"sources": [{"url": "missing.yaml"}]})
        self.assertEqual(self.sync(), first)

    def test_a_failed_download_names_the_index_and_the_offline_option(self):
        self.write_yaml(
            self.modules / "sources.yaml", {"sources": [{"url": "https://x.invalid/i.yaml"}]}
        )
        self.configure(["team/A"])
        failure = requests.ConnectionError("refused")
        with (
            mock.patch("requests.get", side_effect=failure),
            self.assertRaisesMessage(
                SourceUnavailable,
                "https://x.invalid/i.yaml: download failed (cannot connect); `xrobot setup "
                "--offline` works without network when xrobot.lock matches Modules/modules.yaml "
                "and the Modules are checked out",
            ),
        ):
            self.sync()

    def test_requests_use_canonical_ids_and_short_names_must_be_unambiguous(self):
        self.upstream("first/A")
        self.upstream("second/A")
        self.configure(["A"])
        with self.assertRaisesMessage(ValueError, "Expected canonical owner/repo: 'A'"):
            self.sync()
        manager = SourceManager(self.modules / "sources.yaml")
        with self.assertRaisesMessage(ValueError, "Ambiguous package A; specify first/A, second/A"):
            manager.resolve_id("A")
        self.assertEqual(manager.resolve_id("first/a"), "first/A")
        self.configure(["first/A"])
        self.assertEqual(set(self.sync()["modules"]), {"first/A"})

    def test_bsp_entries_are_not_dependencies(self):
        self.upstream("team/Board", kind="bsp")
        self.configure(["team/Board"])
        with self.assertRaisesMessage(
            ValueError, "team/Board is a BSP in the Sources, not a Module dependency"
        ):
            self.sync()
        self.assertFalse((self.modules / "team/Board").exists())

    def test_a_module_before_1_0_is_named_with_the_resolved_commit(self):
        # 以前只列出不支持的 manifest 键，看不出选中的是哪个提交、BSP 该改请求哪个 ref。
        # Only the unsupported manifest keys used to be listed, without the commit that was
        # picked or the ref the BSP should request instead.
        old = self.upstream("team/Old")
        self.write(
            old / "Old.hpp",
            "#pragma once\n/* === MODULE MANIFEST V2 ===\nmodule_description: old\n"
            "constructor_args:\n  - blink_cycle: 250\n=== END MANIFEST === */\nclass Old {};\n",
        )
        run_git(old, "commit", "-q", "-am", "0.x")
        commit = run_git(old, "rev-parse", "HEAD")
        self.configure(["team/Old@master"])
        with self.assertRaises(ValueError) as context:
            self.sync()
        self.assertIn(
            f"Old.hpp: team/Old@{commit[:12]} predates xrobot 1.0 (manifest key(s) "
            "constructor_args); a BSP has to request a version of the Module made for xrobot "
            "1.0, such as its master branch (`@master`);",
            str(context.exception),
        )

    def test_dependency_cycles_are_reported(self):
        self.upstream("team/A", ["team/B"])
        self.upstream("team/B", ["team/A"])
        self.configure(["team/A"])
        with self.assertRaisesMessage(
            ValueError, "Package dependency cycle: team/A -> team/B -> team/A"
        ):
            self.sync()

    def test_a_ref_that_is_both_branch_and_tag_must_be_qualified(self):
        a = self.upstream("team/A")
        run_git(a, "tag", "dev")
        self.configure(["team/A@dev"])
        with self.assertRaisesMessage(
            ValueError,
            "Modules/modules.yaml requests team/A@dev: dev is both a branch and a tag; write "
            "refs/heads/dev or refs/tags/dev",
        ):
            self.sync()
        self.configure(["team/A@refs/heads/dev"])
        self.assertEqual(self.sync()["modules"]["team/A"]["ref_kind"], "branch")

    def test_unknown_refs_are_reported(self):
        self.upstream("team/A")
        self.configure(["team/A@missing"])
        with self.assertRaisesMessage(
            ValueError,
            "Modules/modules.yaml requests team/A@missing: the repository has no branch, tag "
            "or commit missing",
        ):
            self.sync()

    def test_conflicting_dependency_commits_leave_lock_and_checkouts_unchanged(self):
        b = self.upstream("team/B")
        run_git(b, "tag", "old")
        old = run_git(b, "rev-parse", "HEAD")
        new = self.commit(b, [], "new")
        run_git(b, "tag", "new")
        self.upstream("team/A", ["team/B@old"])
        self.upstream("team/C", ["team/B@new"])
        self.configure(["team/A"])
        self.sync()
        before = self.lock_bytes()
        self.configure(["team/A", "team/C"])
        with self.assertRaisesMessage(
            ValueError, f"Dependency conflict for team/B: {old[:12]} vs {new[:12]} (team/C)"
        ):
            self.sync(update=[])
        self.assertEqual(self.lock_bytes(), before)
        self.assertEqual(self.head("team/B"), old)


class MinimalChange(UpstreamTestCase):
    """请求改变时，只重新解析受影响的模块。
    Only the affected Modules are re-resolved when requests change.
    """

    def setUp(self):
        super().setUp()
        self.b = self.upstream("team/B")
        self.a = self.upstream("team/A", ["team/B"])
        self.configure(["team/A"])
        self.first = self.sync()["modules"]
        self.moved_a = self.commit(self.a, ["team/B"], "a moves")
        self.moved_b = self.commit(self.b, [], "b moves")

    def test_adding_a_request_resolves_only_the_new_module(self):
        c = self.upstream("team/C")
        self.configure(["team/A", "team/C"])
        lock = self.sync()["modules"]
        self.assertEqual(lock["team/A"]["commit"], self.first["team/A"]["commit"])
        self.assertEqual(lock["team/B"]["commit"], self.first["team/B"]["commit"])
        self.assertEqual(lock["team/C"]["commit"], run_git(c, "rev-parse", "HEAD"))

    def test_removing_a_request_drops_only_its_modules(self):
        self.upstream("team/C")
        self.configure(["team/A", "team/C"])
        self.sync()
        self.configure(["team/C"])
        lock = self.sync()
        self.assertEqual(set(lock["modules"]), {"team/C"})
        self.assertIn("team/C", (self.modules / "CMakeLists.txt").read_text(encoding="utf-8"))
        self.assertNotIn("team/A", (self.modules / "CMakeLists.txt").read_text(encoding="utf-8"))

    def test_update_moves_the_named_locked_modules_or_all_of_them(self):
        lock = self.sync(update=["team/B"])["modules"]
        self.assertEqual(
            (lock["team/A"]["commit"], lock["team/B"]["commit"]),
            (self.first["team/A"]["commit"], self.moved_b),
        )
        lock = self.sync(update=["A"])["modules"]
        self.assertEqual(lock["team/A"]["commit"], self.moved_a)
        with self.assertRaises(ValueError) as context:
            self.sync(update=["team/Z"])
        self.assertEqual(
            str(context.exception),
            "team/Z is not in xrobot.lock; `--update` takes Module ids from the lock",
        )
        a = self.commit(self.a, ["team/B"], "a moves again")
        b = self.commit(self.b, [], "b moves again")
        lock = self.sync(update=[])["modules"]
        self.assertEqual((lock["team/A"]["commit"], lock["team/B"]["commit"]), (a, b))

    def test_a_new_module_requiring_a_locked_module_elsewhere_suggests_update(self):
        run_git(self.b, "tag", "v2")
        self.upstream("team/C", ["team/B@v2"])
        before = self.lock_bytes()
        self.configure(["team/A", "team/C"])
        with self.assertRaisesMessage(
            ValueError,
            f"team/C requires team/B at v2, but xrobot.lock keeps "
            f"{self.first['team/B']['commit'][:12]}; run `xrobot setup --update team/B`",
        ):
            self.sync()
        self.assertEqual(self.lock_bytes(), before)
        lock = self.sync(update=["team/B"])["modules"]
        self.assertEqual(lock["team/B"]["commit"], self.moved_b)

    def test_changing_the_request_of_a_locked_module_re_resolves_it(self):
        run_git(self.a, "tag", "v1", self.first["team/A"]["commit"])
        self.configure(["team/A@master"])
        self.assertEqual(self.sync()["modules"]["team/A"]["commit"], self.moved_a)
        self.configure(["team/A@v1"])
        self.assertEqual(self.sync()["modules"]["team/A"]["commit"], self.first["team/A"]["commit"])


class LocalWork(UpstreamTestCase):
    """模块检出中的本地修改和本地提交。
    Local changes and local commits in Module checkouts.
    """

    def setUp(self):
        super().setUp()
        self.a = self.upstream("team/A")
        self.configure(["team/A"])
        self.locked = self.sync()["modules"]["team/A"]["commit"]
        self.commit(self.a, [], "upstream moves")

    def test_uncommitted_changes_are_never_overwritten(self):
        header = self.modules / "team/A/A.hpp"
        header.write_bytes(header.read_bytes() + b"\n// local work\n")
        before = header.read_bytes()
        with self.assertRaisesMessage(
            ValueError,
            "team/A has uncommitted changes; they are kept, but the lock cannot move it. "
            "Commit and push them, or discard them, first",
        ):
            self.sync(update=[])
        self.assertEqual(header.read_bytes(), before)

    def test_uncommitted_changes_at_the_locked_commit_are_left_alone(self):
        header = self.modules / "team/A/A.hpp"
        header.write_bytes(header.read_bytes() + b"\n// local work\n")
        before = header.read_bytes()
        for flags in ({}, {"frozen": True}):
            with self.subTest(flags=flags):
                self.assertEqual(self.sync(**flags)["modules"]["team/A"]["commit"], self.locked)
                self.assertEqual(header.read_bytes(), before)

    def test_frozen_fetches_a_locked_commit_the_checkout_lacks(self):
        # 另一位开发者更新了 lock，本地检出里还没有新的 commit。
        # Another developer updated the lock; the local checkout does not have the commit yet.
        newer = self.commit(self.a, [], "newer")
        lock = yaml.safe_load(self.lock_bytes())
        lock["modules"]["team/A"]["commit"] = newer
        self.write_yaml(self.root / "xrobot.lock", lock)
        self.sync(frozen=True)
        self.assertEqual(self.head("team/A"), newer)

    def test_a_checkout_at_its_locked_commit_is_not_queried(self):
        import xrobot.lock as lock_module

        calls = []
        real = lock_module.git

        def spy(folder, *args, **kwargs):
            calls.append(args[0])
            return real(folder, *args, **kwargs)

        with (
            mock.patch.object(lock_module, "git", spy),
            mock.patch.object(lock_module, "checkout_state") as state,
        ):
            self.sync(frozen=True)
        state.assert_not_called()
        self.assertEqual([c for c in calls if c in ("status", "symbolic-ref", "rev-parse")], [])

    def test_an_unpushed_local_commit_is_not_moved(self):
        folder = self.modules / "team/A"
        run_git(folder, "checkout", "-q", "-b", "work")
        (folder / "A.hpp").write_bytes((folder / "A.hpp").read_bytes() + b"\n// local\n")
        run_git(folder, "commit", "-q", "-am", "local")
        local = run_git(folder, "rev-parse", "HEAD")
        before = self.lock_bytes()
        # 不加 --leave-local 仍然拒绝：报错提示该选项，并说明本地提交的去向
        # （保留在原分支或 reflog 中，不删除）。
        # Without --leave-local the refusal stays, with the option in the error and where
        # the local commits end up (they stay on the original branch or in the reflog, they
        # are not deleted). The target is the locked commit for --frozen and the resolved
        # one for --update.
        for flags, target in (
            ({"update": []}, run_git(self.a, "rev-parse", "master")),
            ({"frozen": True}, self.locked),
        ):
            with (
                self.subTest(flags=flags),
                self.assertRaisesMessage(
                    ValueError,
                    f"team/A is at local commit {local[:12]} that is not on any remote branch or "
                    "tag. While developing a module, keep your changes uncommitted; when they are "
                    "ready, push them to a branch of the module and run "
                    f"`xrobot setup --update team/A`, or run `xrobot setup --leave-local "
                    f"team/A` to move it to {target[:12]}; the local commits stay on the "
                    "original branch or in the reflog",
                ),
            ):
                self.sync(**flags)
            self.assertEqual(self.head("team/A"), local)
        self.assertEqual(self.lock_bytes(), before)

    def test_leave_local_moves_only_the_named_modules(self):
        b = self.upstream("team/B")
        self.configure(["team/A", "team/B"])
        self.sync()
        self.commit(self.a, [], "newer A")
        self.commit(b, [], "newer B")
        for identity in ("team/A", "team/B"):
            folder = self.modules / identity
            name = identity.rsplit("/", 1)[-1]
            run_git(folder, "checkout", "-q", "-b", "work")
            (folder / (name + ".hpp")).write_bytes(
                (folder / (name + ".hpp")).read_bytes() + b"\n// local\n"
            )
            run_git(folder, "commit", "-q", "-am", "local")
        local_a, local_b = self.head("team/A"), self.head("team/B")
        branches = {
            identity: run_git(
                self.modules / identity, "for-each-ref", "refs/heads", "--format=%(refname)"
            )
            for identity in ("team/A", "team/B")
        }
        # 未点名的模块仍然拒绝，报错提示 --leave-local，并说明本地提交的去向。
        # An unnamed Module is still refused, with the --leave-local hint and where the
        # local commits end up.
        with self.assertRaisesMessage(
            ValueError,
            f"team/B is at local commit {local_b[:12]} that is not on any remote branch or "
            "tag. While developing a module, keep your changes uncommitted; when they are "
            "ready, push them to a branch of the module and run "
            f"`xrobot setup --update team/B`, or run `xrobot setup --leave-local team/B` "
            f"to move it to {run_git(b, 'rev-parse', 'master')[:12]}; the local commits "
            "stay on the original branch or in the reflog",
        ):
            self.sync(update=[], leave_local=["team/A"])
        self.assertEqual(self.head("team/A"), local_a)
        self.assertEqual(self.head("team/B"), local_b)
        # 点名的模块直接检出目标提交：先打印移动的提交、改动文件和本地提交的去向，不建备份分支。
        # A named Module is checked out at the target directly: the moved commits, their
        # files and where the local commits end up are printed first, without a backup
        # branch.
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            moved = self.sync(update=[], leave_local=["team/A", "team/B"])
        text = output.getvalue()
        for identity, local in (("team/A", local_a), ("team/B", local_b)):
            self.assertNotEqual(moved["modules"][identity]["commit"], local)
            self.assertEqual(self.head(identity), moved["modules"][identity]["commit"])
            self.assertIn(local[:7], text)
            self.assertIn(identity.rsplit("/", 1)[-1] + ".hpp", text)
            self.assertEqual(
                run_git(
                    self.modules / identity, "for-each-ref", "refs/heads", "--format=%(refname)"
                ),
                branches[identity],
            )

    def test_leave_local_keeps_the_commits_on_their_local_branch(self):
        folder = self.modules / "team/A"
        run_git(folder, "checkout", "-q", "-b", "work")
        (folder / "A.hpp").write_bytes((folder / "A.hpp").read_bytes() + b"\n// local\n")
        run_git(folder, "commit", "-q", "-am", "local")
        local = self.head("team/A")
        target = run_git(self.a, "rev-parse", "master")
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            moved = self.sync(update=[], leave_local=["team/A"])
        # 移动后模块停在目标提交上，分支 work 原封不动，仍指向原本地提交。
        # After the move the Module sits at the target commit, and the branch work is
        # untouched, still pointing at the original local commit.
        self.assertEqual(moved["modules"]["team/A"]["commit"], target)
        self.assertEqual(self.head("team/A"), target)
        self.assertEqual(run_git(folder, "rev-parse", "work"), local)
        # 打印提到本地提交留在了哪个分支。
        # The print names the branch the local commits stay on.
        self.assertIn("work", output.getvalue())

    def test_leave_local_names_the_reflog_for_a_detached_head(self):
        folder = self.modules / "team/A"
        # 克隆留下的检出在 master 分支上；先游离再提交，模拟直接停在提交上的检出。
        # The checkout left by the clone sits on master; detach before committing to model
        # a checkout parked directly at a commit.
        run_git(folder, "checkout", "-q", "--detach")
        (folder / "A.hpp").write_bytes((folder / "A.hpp").read_bytes() + b"\n// local\n")
        run_git(folder, "commit", "-q", "-am", "local")
        local = self.head("team/A")
        target = run_git(self.a, "rev-parse", "master")
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            moved = self.sync(update=[], leave_local=["team/A"])
        self.assertEqual(moved["modules"]["team/A"]["commit"], target)
        # 游离 HEAD 上的本地提交只能从 reflog 找回：打印提到 reflog 和完整的原 HEAD 提交号。
        # Local commits of a detached HEAD are only recoverable through the reflog: the
        # print names it and gives the full original HEAD commit.
        text = output.getvalue()
        self.assertIn("reflog", text)
        self.assertIn(local, text)

    def test_force_alone_is_refused_before_anything_moves(self):
        folder = self.modules / "team/A"
        run_git(folder, "checkout", "-q", "-b", "work")
        (folder / "A.hpp").write_bytes((folder / "A.hpp").read_bytes() + b"\n// local\n")
        run_git(folder, "commit", "-q", "-am", "local")
        local = self.head("team/A")
        # 单独给 --force 在动任何模块之前报错：提示需要配合 --leave-local 点名模块。
        # --force alone fails before any Module is touched: the hint says to combine it
        # with --leave-local naming the Modules.
        with self.assertRaisesMessage(
            ValueError,
            "--force only works with --leave-local; it affects only the Modules named by "
            "`xrobot setup --leave-local <module>`",
        ):
            self.sync(update=[], force=True)
        self.assertEqual(self.head("team/A"), local)

    def test_force_discards_the_uncommitted_changes_of_the_named_module(self):
        folder = self.modules / "team/A"
        header = folder / "A.hpp"
        pristine = header.read_bytes()
        header.write_bytes(pristine + b"\n// tracked work\n")
        (folder / "new.txt").write_text("untracked\n", encoding="utf-8")
        exclude = folder / ".git" / "info" / "exclude"
        exclude.write_bytes(exclude.read_bytes() + b"ignored.txt\n")
        (folder / "ignored.txt").write_text("ignored\n", encoding="utf-8")
        target = run_git(self.a, "rev-parse", "master")
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            moved = self.sync(update=[], leave_local=["team/A"], force=True)
        # 模块移到锁定提交：已跟踪的修改被还原、未跟踪的新文件被删除、被忽略的文件仍在。
        # The Module moves to the locked commit: the tracked modification is reverted, the
        # untracked file is removed, the ignored file is kept.
        self.assertEqual(moved["modules"]["team/A"]["commit"], target)
        self.assertEqual(self.head("team/A"), target)
        self.assertNotIn(b"// tracked work", header.read_bytes())
        self.assertFalse((folder / "new.txt").exists())
        self.assertTrue((folder / "ignored.txt").exists())
        # 打印列出了被丢弃的文件，不含被忽略的文件。
        # The print lists the discarded files, without the ignored one.
        text = output.getvalue()
        self.assertIn("A.hpp", text)
        self.assertIn("new.txt", text)
        self.assertNotIn("ignored.txt", text)

    def test_leave_local_refuses_uncommitted_changes_without_force(self):
        folder = self.modules / "team/A"
        run_git(folder, "checkout", "-q", "-b", "work")
        (folder / "A.hpp").write_bytes((folder / "A.hpp").read_bytes() + b"\n// local\n")
        run_git(folder, "commit", "-q", "-am", "local")
        header = folder / "A.hpp"
        header.write_bytes(header.read_bytes() + b"\n// more work\n")
        before = header.read_bytes()
        # 点名的模块有修改但没给 --force 时照旧拒绝：提示 --force 会丢弃哪些内容。
        # A named Module with changes is still refused without --force: the hint says what
        # --force would discard.
        with self.assertRaisesMessage(
            ValueError,
            "team/A has uncommitted changes; the lock cannot move it. Commit and push them, "
            "or rerun with `--force` to discard the tracked modifications and untracked files "
            "(ignored files are kept)",
        ):
            self.sync(update=[], leave_local=["team/A"])
        self.assertEqual(header.read_bytes(), before)

    def test_force_does_not_touch_the_changes_of_unnamed_modules(self):
        b = self.upstream("team/B")
        self.configure(["team/A", "team/B"])
        self.sync()
        self.commit(self.a, [], "newer A")
        self.commit(b, [], "newer B")
        folder = self.modules / "team/A"
        run_git(folder, "checkout", "-q", "-b", "work")
        (folder / "A.hpp").write_bytes((folder / "A.hpp").read_bytes() + b"\n// local\n")
        run_git(folder, "commit", "-q", "-am", "local")
        local = self.head("team/A")
        header = self.modules / "team/B" / "B.hpp"
        header.write_bytes(header.read_bytes() + b"\n// local work\n")
        before = header.read_bytes()
        # --force 只作用于点名的模块：没点名的模块有修改时照旧拒绝，文件不变。
        # --force affects only the named Modules: an unnamed Module with changes is still
        # refused, and its files stay as they are.
        with self.assertRaisesMessage(
            ValueError,
            "team/B has uncommitted changes; they are kept, but the lock cannot move it. "
            "Commit and push them, or discard them, first",
        ):
            self.sync(update=[], leave_local=["team/A"], force=True)
        self.assertEqual(self.head("team/A"), local)
        self.assertEqual(header.read_bytes(), before)

    def test_force_leaves_a_named_module_at_its_locked_commit_alone(self):
        header = self.modules / "team/A" / "A.hpp"
        header.write_bytes(header.read_bytes() + b"\n// local work\n")
        before = header.read_bytes()
        # 点名但不需要移动的模块（已在锁定提交上）即使有 --force 也不动它的修改。
        # A named Module that does not have to move (already at its locked commit) keeps
        # its changes even with --force.
        self.assertEqual(
            self.sync(leave_local=["team/A"], force=True)["modules"]["team/A"]["commit"],
            self.locked,
        )
        self.assertEqual(header.read_bytes(), before)

    def test_a_name_matching_several_locked_modules_is_ambiguous(self):
        self.upstream("other/a")
        self.configure(["team/A", "other/a"])
        self.sync()
        # 末段匹配忽略大小写：team/A 的短名 A 同时命中 other/a 和 team/A，报歧义并列出
        # 命中的 id，而不是说 lock 中没有。
        # The last-segment match folds the case, so the short name A of team/A also matches
        # other/a: report the match as ambiguous with the ids listed, not as missing.
        with self.assertRaisesMessage(ValueError, "Ambiguous Module A; specify other/a, team/A"):
            self.sync(update=[], leave_local=["A"])
        # 一个模块都没命中的名字仍然报 lock 中没有。
        # A name matching no Module still reports that it is not in the lock.
        with self.assertRaisesMessage(
            ValueError,
            "missing is not in xrobot.lock; `--leave-local` takes Module ids from the lock",
        ):
            self.sync(update=[], leave_local=["missing"])

    def test_update_a_name_matching_several_locked_modules_is_ambiguous(self):
        self.upstream("other/a")
        self.configure(["team/A", "other/a"])
        self.sync()
        # 与 --leave-local 一样，--update 点名命中多个模块时报歧义并列出命中的 id，
        # 而不是说 lock 中没有。
        # Like --leave-local, --update reports a name matching several locked Modules as
        # ambiguous with the ids listed, not as missing.
        with self.assertRaisesMessage(ValueError, "Ambiguous Module A; specify other/a, team/A"):
            self.sync(update=["A"])
        # 一个模块都没命中的名字仍然报 lock 中没有。
        # A name matching no Module still reports that it is not in the lock.
        with self.assertRaisesMessage(
            ValueError,
            "missing is not in xrobot.lock; `--update` takes Module ids from the lock",
        ):
            self.sync(update=["missing"])

    def test_generation_rejects_a_checkout_away_from_the_lock_with_the_fix(self):
        from xrobot.project import Project

        folder = self.modules / "team/A"
        run_git(folder, "fetch", "-q", "origin")
        run_git(folder, "checkout", "-q", "--detach", "origin/master")
        with self.assertRaisesMessage(
            ValueError,
            f"team/A is checked out at {run_git(folder, 'rev-parse', 'HEAD')[:12]} but "
            f"xrobot.lock pins {self.locked[:12]}. While developing a module, keep your changes "
            "uncommitted; when they are ready, push them to a branch of the module and run "
            "`xrobot setup --update team/A`. To return to the locked sources run `xrobot setup`.",
        ):
            discover_modules(self.modules, Project(self.root).lock)


class Contexts(UpstreamTestCase):
    """same/same-or-dev 请求跟随的分支或 tag。
    The branch or tag that same/same-or-dev requests follow.
    """

    def test_same_or_dev_follows_a_stacked_branch_then_falls_back_to_dev(self):
        b = self.upstream("team/B")
        a = self.upstream("team/A", [{"id": "team/B", "ref": "same-or-dev"}])
        run_git(b, "checkout", "-q", "-b", "feature/next")
        feature = self.commit(b, [], "feature")
        run_git(a, "checkout", "-q", "-b", "feature/next")
        self.configure(["team/A@feature/next"])
        first = self.sync()
        self.assertEqual(first["modules"]["team/B"]["commit"], feature)
        self.assertEqual(first["modules"]["team/B"]["resolved_ref"], "feature/next")
        run_git(b, "checkout", "-q", "dev")
        run_git(b, "merge", "-q", "--ff-only", "feature/next")
        run_git(b, "branch", "-q", "-D", "feature/next")
        second = self.sync(update=[])
        self.assertEqual(second["modules"]["team/B"]["commit"], feature)
        self.assertEqual(second["modules"]["team/B"]["resolved_ref"], "dev")
        self.assertEqual(
            first["modules"]["team/A"]["commit"], second["modules"]["team/A"]["commit"]
        )

    def test_same_or_dev_keeps_the_original_context_through_the_chain(self):
        """三层链上中间层退回 dev 时，下一层仍跟随最初的功能分支。
        When a middle layer of a three-layer chain falls back to dev, the next layer still
        follows the original feature branch.
        """
        c = self.upstream("team/C")
        self.upstream("team/B", [{"id": "team/C", "ref": "same-or-dev"}])
        a = self.upstream("team/A", [{"id": "team/B", "ref": "same-or-dev"}])
        run_git(c, "checkout", "-q", "-b", "feature/next")
        feature = self.commit(c, [], "feature of C")
        # team/A 的功能分支与 master 内容相同，依赖清单不变。
        # The feature branch of team/A has the content of master, dependencies included.
        run_git(a, "checkout", "-q", "-b", "feature/next")
        run_git(self.root, "init", "-q", "-b", "feature/next")
        self.configure(["team/A@same-or-dev", "team/C@same-or-dev"])
        result = self.sync()
        # team/B 没有同名分支，退回 dev；team/C 经两条路径都跟随最初的功能分支。
        # team/B has no branch of that name and falls back to dev; team/C follows the
        # original feature branch through both paths.
        self.assertEqual(result["modules"]["team/B"]["resolved_ref"], "dev")
        self.assertEqual(result["modules"]["team/C"]["commit"], feature)
        self.assertEqual(result["modules"]["team/C"]["resolved_ref"], "feature/next")
        self.assertEqual(result["modules"]["team/A"]["commit"], run_git(a, "rev-parse", "HEAD"))

    def test_an_explicit_tag_or_commit_keeps_the_original_context_for_its_dependencies(self):
        """显式 tag 或提交号的依赖，其 same-or-dev 依赖仍跟随最初的上下文。
        The same-or-dev dependencies of an explicitly requested tag or commit still follow
        the original context.
        """
        self.upstream("team/C")
        b = self.upstream("team/B", [{"id": "team/C", "ref": "same-or-dev"}])
        a = self.upstream("team/A", [{"id": "team/B", "ref": "same-or-dev"}])
        # team/A 的功能分支与 master 内容相同并被 tag；team/B 的功能分支有一个新提交。
        # The feature branch of team/A has the content of master and a tag; the feature
        # branch of team/B has a new commit.
        run_git(a, "checkout", "-q", "-b", "feature/next")
        run_git(b, "checkout", "-q", "-b", "feature/next")
        feature = self.commit(b, [], "feature of B")
        run_git(a, "tag", "release-check")
        run_git(self.root, "init", "-q", "-b", "feature/next")
        for pinned in ("release-check", run_git(a, "rev-parse", "HEAD")):
            with self.subTest(pinned=pinned):
                (self.root / "xrobot.lock").unlink(missing_ok=True)
                self.configure([f"team/A@{pinned}"])
                result = self.sync(context_ref="refs/heads/feature/next")
                self.assertEqual(
                    result["modules"]["team/A"]["ref_kind"],
                    "tag" if pinned == "release-check" else "commit",
                )
                # team/B 跟随最初的功能分支，而不是去找同名的 tag 或停在提交上。
                # team/B follows the original feature branch instead of looking for a tag of
                # the same name or stopping at the commit.
                self.assertEqual(result["modules"]["team/B"]["commit"], feature)
                self.assertEqual(result["modules"]["team/B"]["resolved_ref"], "feature/next")

    def test_a_conflict_names_the_repository_that_fell_back_to_dev(self):
        """依赖冲突的报错写明链上哪个仓库缺同名分支、退回了 dev。
        The conflict error names the repository in the chain that lacks the branch and fell
        back to dev.
        """
        c = self.upstream("team/C")
        self.upstream("team/B", [{"id": "team/C", "ref": "same-or-dev"}])
        a = self.upstream("team/A", [{"id": "team/B", "ref": "same-or-dev"}])
        dev = run_git(c, "rev-parse", "HEAD")
        run_git(c, "checkout", "-q", "-b", "feature/next")
        feature = self.commit(c, [], "feature of C")
        run_git(a, "checkout", "-q", "-b", "feature/next")
        run_git(self.root, "init", "-q", "-b", "feature/next")
        self.configure(["team/C@dev", "team/A@same-or-dev"])
        with self.assertRaisesMessage(
            ValueError,
            f"Dependency conflict for team/C: {dev[:12]} vs {feature[:12]} "
            "(team/A -> team/B); team/B has no feature/next branch and falls back to dev",
        ):
            self.sync()

    def test_a_tag_never_falls_back_to_dev(self):
        b = self.upstream("team/B")
        a = self.upstream("team/A", [{"id": "team/B", "ref": "same-or-dev"}])
        run_git(a, "tag", "2026-09-15")
        self.configure(["team/A@2026-09-15"])
        with self.assertRaisesMessage(
            ValueError, "team/A requests team/B@same-or-dev: the repository has no tag 2026-09-15"
        ):
            self.sync()
        self.assertFalse((self.root / "xrobot.lock").exists())
        run_git(b, "tag", "2026-09-15")
        row = self.sync()["modules"]["team/B"]
        self.assertEqual((row["ref_kind"], row["resolved_ref"]), ("tag", "2026-09-15"))

    def test_a_dependency_without_the_branch_or_dev_is_reported(self):
        self.upstream("team/B", branches=())
        a = self.upstream("team/A", [{"id": "team/B", "ref": "same-or-dev"}])
        run_git(a, "branch", "feature/next")
        for line, missing in (
            ("feature/next", "neither a feature/next branch nor a dev branch"),
            ("dev", "no dev branch"),
        ):
            with self.subTest(line=line):
                self.configure(["team/A@" + line])
                with self.assertRaisesMessage(
                    ValueError,
                    f"team/A requests team/B@same-or-dev: the repository has {missing}; request "
                    "an explicit tag, commit or branch",
                ):
                    self.sync()

    def test_a_detached_commit_request_needs_a_logical_context_for_its_dependencies(self):
        self.upstream("team/B")
        a = self.upstream("team/A", [{"id": "team/B", "ref": "same-or-dev"}])
        sha = run_git(a, "rev-parse", "HEAD")
        self.configure(["team/A@" + sha])
        with self.assertRaisesMessage(
            ValueError,
            "team/A requests team/B@same-or-dev: there is no branch or tag to follow: the BSP "
            "checkout is not on a branch, or the requesting Module is pinned to a commit; pass "
            "--context-ref refs/heads/<branch>",
        ):
            self.sync()
        self.configure([{"id": "team/A", "ref": sha, "context_ref": "refs/heads/pr-feature"}])
        self.assertEqual(self.sync()["modules"]["team/B"]["resolved_ref"], "dev")

    def test_root_same_or_dev_uses_the_bsp_branch_only_while_resolving(self):
        a = self.upstream("team/A")
        run_git(a, "branch", "feature/board")
        run_git(self.root, "init", "-q", "-b", "feature/board")
        self.configure(["team/A@same-or-dev"])
        first = self.sync()
        self.assertEqual(first["modules"]["team/A"]["resolved_ref"], "feature/board")
        self.assertNotIn("context", first["modules"]["team/A"])
        self.assertEqual(self.sync(offline=True), first)
        run_git(self.root, "symbolic-ref", "HEAD", "refs/heads/other")
        self.assertEqual(self.sync(frozen=True), first)
        self.assertEqual(self.sync(update=[])["modules"]["team/A"]["resolved_ref"], "dev")

    def test_a_tag_context_is_exact(self):
        a = self.upstream("team/A")
        self.configure(["team/A@same-or-dev"])
        with self.assertRaisesMessage(
            ValueError,
            "Modules/modules.yaml requests team/A@same-or-dev: the repository has no tag "
            "release-check",
        ):
            self.sync(context_ref="refs/tags/release-check")
        run_git(a, "tag", "release-check")
        first = self.sync(context_ref="refs/tags/release-check")
        self.assertEqual(first["modules"]["team/A"]["ref_kind"], "tag")
        self.assertEqual(self.sync(context_ref="refs/tags/release-check", offline=True), first)

    def test_outside_git_same_or_dev_keeps_the_locked_commit(self):
        a = self.upstream("team/A")
        self.upstream("team/C")
        self.configure(["team/A@same-or-dev"])
        with self.assertRaisesMessage(
            ValueError,
            "Modules/modules.yaml requests team/A@same-or-dev, which needs the BSP branch to "
            "pick a commit, but the BSP is not a Git repository; request an explicit ref such "
            "as team/A@master, or put the BSP in a Git repository",
        ):
            self.sync()
        first = self.sync(context_ref="refs/heads/review")
        self.assertEqual(first["modules"]["team/A"]["resolved_ref"], "dev")
        run_git(a, "checkout", "-q", "dev")
        self.commit(a, [], "newer dev")
        self.configure(["team/A@same-or-dev", "team/C@dev"])
        # 不带 --update 沿用 lock，不再打警告。
        # Without --update the lock is kept and nothing is warned about.
        second = self.sync()
        self.assertEqual(second["modules"]["team/A"], first["modules"]["team/A"])
        self.assertEqual(second["modules"]["team/C"]["resolved_ref"], "dev")
        # --update 要按 BSP 的分支重新解析：报错并且不改动任何文件。
        # --update would re-resolve by the BSP branch: it fails and changes no file.
        before = self.lock_bytes()
        head = self.head("team/A")
        with self.assertRaisesMessage(
            ValueError,
            "`--update` would resolve team/A@same-or-dev by following the BSP branch, but the "
            "BSP is not a Git repository; pass --context-ref refs/heads/<branch>",
        ):
            self.sync(update=[])
        self.assertEqual(self.lock_bytes(), before)
        self.assertEqual(self.head("team/A"), head)
        self.assertEqual(self.sync(update=["team/C"])["modules"]["team/C"]["resolved_ref"], "dev")

    def test_a_detached_bsp_checkout_needs_a_context_ref(self):
        self.upstream("team/A")
        run_git(self.root, "init", "-q")
        run_git(self.root, "commit", "-q", "--allow-empty", "-m", "bsp")
        run_git(self.root, "checkout", "-q", "--detach")
        self.configure(["team/A@same-or-dev"])
        with self.assertRaisesMessage(
            ValueError,
            "Modules/modules.yaml requests team/A@same-or-dev, which follows the BSP branch, "
            "but the BSP checkout is not on a branch (detached HEAD); pass --context-ref "
            "refs/heads/<branch>",
        ):
            self.sync()

    def test_a_request_can_carry_its_own_context(self):
        self.upstream("team/A")
        self.configure([{"id": "team/A", "ref": "same", "context_ref": "refs/heads/dev"}])
        first = self.sync()
        self.assertEqual(first["modules"]["team/A"]["resolved_ref"], "dev")
        self.assertEqual(self.sync(offline=True), first)

    def test_context_refs_must_be_qualified(self):
        self.upstream("team/A")
        self.configure(["team/A@same-or-dev"])
        with self.assertRaisesMessage(
            ValueError, "A context/release ref must start with refs/heads/ or refs/tags/"
        ):
            self.sync(context_ref="dev")


class LockFile(UpstreamTestCase):
    """xrobot.lock 的写法：可移植的地址和固定的格式。
    How xrobot.lock is written: portable locations and a fixed layout.
    """

    def test_relative_index_and_local_sources_are_stored_relative_to_the_lock(self):
        self.upstream("team/A")
        self.entries[0]["repo"] = "upstream/team/A"
        self.write_index()
        self.write_yaml(self.modules / "sources.yaml", {"sources": [{"url": "../../index.yaml"}]})
        self.configure(["team/A"])
        first = self.sync()
        row = first["modules"]["team/A"]
        self.assertEqual((row["repo"], row["source"]), ("../upstream/team/A", "../index.yaml"))
        self.assertNotIn(str(self.tmp), self.lock_bytes().decode("utf-8"))
        self.assertNotIn(self.tmp.as_posix(), self.lock_bytes().decode("utf-8"))
        self.assertEqual(self.sync(offline=True), first)

    def test_a_locked_local_source_stays_relative_to_the_lock_when_run_from_a_subdirectory(self):
        self.upstream("team/A")
        self.entries[0]["repo"] = "upstream/team/A"
        self.write_index()
        self.configure(["team/A"])
        self.sync()
        self.upstream("team/B")
        self.configure(["team/A", "team/B"])
        (self.root / "User").mkdir()
        lock = self.sync(cwd=self.root / "User")
        self.assertEqual(lock["modules"]["team/A"]["repo"], "../upstream/team/A")

    def test_file_urls_are_stored_without_machine_paths(self):
        a = self.upstream("team/A")
        self.entries[0]["repo"] = a.as_uri()
        self.write_index()
        self.configure(["team/A@dev"])
        first = self.sync()
        self.assertEqual(first["modules"]["team/A"]["repo"], "../upstream/team/A")
        self.assertNotIn("file://", self.lock_bytes().decode("utf-8"))
        self.assertEqual(self.sync(offline=True), first)

    def test_the_lock_records_the_canonical_repository_and_fetches_from_a_mirror(self):
        a = self.upstream("team/A")
        mirror = self.tmp / "mirror" / "A"
        run_git(None, "clone", "-q", "--bare", str(a), str(mirror))
        mirror_index = self.write_yaml(
            self.tmp / "mirror.yaml", {"mirror_of": "team", "modules": [mirror.as_uri()]}
        )
        self.write_yaml(
            self.modules / "sources.yaml",
            {
                "sources": [
                    {"url": str(self.index), "priority": 0},
                    {"url": str(mirror_index), "priority": 1},
                ]
            },
        )
        self.configure(["team/A"])
        lock = self.sync()
        row = lock["modules"]["team/A"]
        self.assertEqual(row["repo"], "../upstream/team/A")
        self.assertTrue(
            same_repository(
                run_git(self.modules / "team/A", "remote", "get-url", "origin"), str(mirror)
            )
        )
        # 离线时没有源，镜像检出凭 lock 中的 commit 被接受。
        # Offline there are no Sources; the mirror checkout is accepted by its locked commit.
        self.assertEqual(self.sync(offline=True), lock)

    def test_offline_rejects_another_origin_without_the_locked_commit(self):
        self.upstream("team/A")
        self.configure(["team/A"])
        self.sync()
        other = self.tmp / "other"
        run_git(None, "init", "-q", str(other))
        run_git(self.modules / "team/A", "remote", "set-url", "origin", str(other))
        lock = yaml.safe_load((self.root / "xrobot.lock").read_text(encoding="utf-8"))
        lock["modules"]["team/A"]["commit"] = "0" * 40
        self.write_yaml(self.root / "xrobot.lock", lock)
        with self.assertRaisesMessage(
            ValueError,
            f"Source mismatch for team/A: {other} != {self.tmp / 'upstream/team/A'}",
        ):
            self.sync(offline=True)

    def test_an_incomplete_or_extended_lock_is_rejected(self):
        self.upstream("team/B")
        self.upstream("team/A", ["team/B"])
        self.configure(["team/A"])
        lock = self.sync()
        missing = dict(lock, modules={"team/A": lock["modules"]["team/A"]})
        self.write_yaml(self.root / "xrobot.lock", missing)
        with self.assertRaisesMessage(
            ValueError, "xrobot.lock is missing or ambiguous for team/B; run `xrobot setup`"
        ):
            self.sync(offline=True)
        self.upstream("team/C")
        self.configure(["team/C"])
        self.sync(update=[])
        extended = yaml.safe_load(self.lock_bytes())
        extended["modules"].update(lock["modules"])
        self.write_yaml(self.root / "xrobot.lock", extended)
        with self.assertRaisesMessage(
            ValueError,
            "xrobot.lock contains Modules outside the declared dependency closure: team/A, "
            "team/B; run `xrobot setup`",
        ):
            self.sync(offline=True)

    def test_legacy_and_unknown_lock_formats_are_rejected(self):
        self.upstream("team/A")
        self.configure(["team/A"])
        lock = self.sync()
        legacy = yaml.safe_load(self.lock_bytes())
        legacy["modules"]["team/A"]["directory"] = "team/A"
        self.write_yaml(self.root / "xrobot.lock", legacy)
        with self.assertRaisesMessage(
            ValueError, "Legacy lock entry for team/A; regenerate it with `xrobot setup --update`"
        ):
            self.sync(frozen=True)
        self.write_yaml(self.root / "xrobot.lock", dict(lock, version=2))
        with self.assertRaisesMessage(
            ValueError,
            f"{self.root / 'xrobot.lock'}: unsupported lock format; regenerate it with "
            "`xrobot setup --update`",
        ):
            self.sync(frozen=True)
        broken = dict(lock)
        broken["modules"] = {"team/A": dict(lock["modules"]["team/A"], commit="abc")}
        self.write_yaml(self.root / "xrobot.lock", broken)
        with self.assertRaisesMessage(ValueError, "Invalid locked commit for team/A"):
            self.sync(frozen=True)

    def test_module_list_includes_or_adds_the_folder(self):
        folder = self.modules / "team/Plain"
        folder.mkdir(parents=True)
        (self.modules / "team/WithCMake").mkdir(parents=True)
        self.write(self.modules / "team/WithCMake/CMakeLists.txt", "")
        write_cmake(self.modules, {"team/Plain": {}, "team/WithCMake": {}})
        text = (self.modules / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertTrue(
            text.startswith(
                "# Generated by `xrobot setup` from xrobot.lock; do not edit or commit."
            )
        )
        self.assertIn(
            'target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}/team/Plain")', text
        )
        self.assertIn('include("${CMAKE_CURRENT_LIST_DIR}/team/WithCMake/CMakeLists.txt")', text)


class ReleaseGate(UpstreamTestCase):
    """发布线检查（--release-ref）。
    The release-line check (--release-ref).
    """

    def setUp(self):
        super().setUp()
        self.a = self.upstream("team/A")

    def lock_feature(self):
        """把 team/A 锁定到 feature/x 分支上的提交，返回该提交。
        Lock team/A to a commit on branch feature/x and return that commit.
        """
        run_git(self.a, "checkout", "-q", "-b", "feature/x")
        feature = self.commit(self.a, [], "feature work")
        run_git(self.a, "checkout", "-q", "master")
        self.configure(["team/A@feature/x"])
        self.assertEqual(self.sync()["modules"]["team/A"]["commit"], feature)
        return feature

    def merge(self, branch, *options):
        """在上游仓库的 branch 分支上执行 git merge。
        Run git merge on branch of the upstream repository.
        """
        run_git(self.a, "checkout", "-q", branch)
        run_git(self.a, "merge", "-q", *options)
        run_git(self.a, "checkout", "-q", "master")

    def test_a_failed_fetch_of_the_release_line_is_reported(self):
        self.lock_feature()
        self.a.rename(self.a.with_name("unavailable"))
        with self.assertRaisesRegex(
            ValueError, r"Git failed in .*: fetch origin \+refs/heads/\*:refs/remotes/origin/\*"
        ):
            self.sync(frozen=True, release_ref="refs/heads/dev")

    def test_unmerged_feature_commits_are_refused_for_dev(self):
        feature = self.lock_feature()
        before = self.lock_bytes()
        with self.assertRaisesMessage(
            ValueError,
            f"team/A: locked commit {feature[:12]} is not on dev (feature branch not merged, or "
            "merged by squash/rebase); after merging the Module, run `xrobot setup --update "
            "team/A --context-ref refs/heads/dev`",
        ):
            self.sync(frozen=True, release_ref="refs/heads/dev")
        self.assertEqual(self.lock_bytes(), before)

    def test_feature_commits_merged_into_dev_pass_for_dev_but_not_for_master(self):
        feature = self.lock_feature()
        self.merge("dev", "--no-ff", "-m", "merge feature", "feature/x")
        self.assertEqual(
            self.sync(frozen=True, release_ref="refs/heads/dev")["modules"]["team/A"]["commit"],
            feature,
        )
        for target in ("refs/heads/master", "refs/heads/main", "refs/tags/v1.0.0"):
            with (
                self.subTest(target=target),
                self.assertRaisesMessage(
                    ValueError,
                    f"team/A: locked commit {feature[:12]} is not on master "
                    "(feature branch not merged, or merged by squash/rebase); after merging the Module, run "
                    "`xrobot setup --update team/A --context-ref refs/heads/master`",
                ),
            ):
                self.sync(frozen=True, release_ref=target)

    def test_squash_merged_feature_commits_are_reported(self):
        feature = self.lock_feature()
        run_git(self.a, "checkout", "-q", "dev")
        run_git(self.a, "merge", "-q", "--squash", "feature/x")
        run_git(self.a, "commit", "-q", "-m", "squash feature")
        run_git(self.a, "checkout", "-q", "master")
        with self.assertRaisesMessage(
            ValueError,
            f"team/A: locked commit {feature[:12]} is not on dev "
            "(feature branch not merged, or merged by squash/rebase); after merging the Module, run "
            "`xrobot setup --update team/A --context-ref refs/heads/dev`",
        ):
            self.sync(frozen=True, release_ref="refs/heads/dev")
        self.configure(["team/A@dev"])
        self.assertEqual(
            self.sync(release_ref="refs/heads/dev")["modules"]["team/A"]["resolved_ref"], "dev"
        )

    def test_master_hotfix_commits_are_refused_for_dev(self):
        hotfix = self.commit(self.a, [], "hotfix")
        self.configure(["team/A@master"])
        self.assertEqual(
            self.sync(release_ref="refs/heads/master")["modules"]["team/A"]["commit"], hotfix
        )
        with self.assertRaisesMessage(
            ValueError,
            f"team/A: locked commit {hotfix[:12]} is not on dev "
            "(feature branch not merged, or merged by squash/rebase); after merging the Module, run "
            "`xrobot setup --update team/A --context-ref refs/heads/dev`",
        ):
            self.sync(frozen=True, release_ref="refs/heads/dev")

    def test_other_targets_are_not_gated(self):
        self.lock_feature()
        self.sync(frozen=True, release_ref="refs/heads/feature/board")

    def test_an_explicitly_requested_tag_is_released(self):
        feature = self.lock_feature()
        run_git(self.a, "tag", "2026-09-28", feature)
        self.configure(["team/A@2026-09-28"])
        self.assertEqual(
            self.sync(release_ref="refs/tags/v1.0.0")["modules"]["team/A"]["commit"], feature
        )

    def test_a_main_only_module_is_checked_against_main(self):
        main = self.upstream("team/M", branches=())
        run_git(main, "branch", "-q", "-m", "master", "main")
        self.configure(["team/M@main"])
        self.assertEqual(
            self.sync(release_ref="refs/heads/master")["modules"]["team/M"]["ref_kind"], "branch"
        )

    def test_a_module_without_the_line_needs_a_commit_or_tag_pin(self):
        third = self.upstream("other/T", branches=())
        self.configure(["other/T@master"])
        with self.assertRaisesMessage(
            ValueError, "other/T has no dev branch; request an explicit tag or commit"
        ):
            self.sync(release_ref="refs/heads/dev")
        sha = run_git(third, "rev-parse", "HEAD")
        self.configure(["other/T@" + sha])
        self.assertEqual(
            self.sync(release_ref="refs/heads/dev")["modules"]["other/T"]["commit"], sha
        )


class ToolPins(UpstreamTestCase):
    """工具固定版本的发布线检查。
    The release-line check of tool pins.
    """

    def setUp(self):
        super().setUp()
        self.tool = self.upstream("xrobot-org/XRobot", listed=False)
        self.merged = run_git(self.tool, "rev-parse", "HEAD")
        run_git(self.tool, "checkout", "-q", "dev")
        self.dev_only = self.commit(self.tool, [], "dev work")
        run_git(self.tool, "checkout", "-q", "-b", "feature/x")
        self.feature = self.commit(self.tool, [], "feature work")
        run_git(self.tool, "checkout", "-q", "master")
        self.cache = self.tmp / "cache"
        run_git(None, "clone", "-q", "--bare", str(self.tool), str(self.cache / "xrobot.git"))

    def check(self, pin, target, offline=False, generator=None):
        """写入工具版本锁定并运行 check_tool_pins。
        Write the tool pins and run check_tool_pins.
        """
        from xrobot.lock import check_tool_pins

        self.configure([], pin=pin)
        if generator is not None:
            self.write(self.root / "User/libxr_config.yaml", f"generator: {generator}\n")
        check_tool_pins(self.project, target, offline, cache=self.cache)

    def test_release_versions_are_released(self):
        self.check("1.0.0", "refs/heads/master")
        self.check("1.0.0", "refs/tags/v1", generator="6.0.0")

    def test_missing_pins_are_errors_for_gated_targets(self):
        with self.assertRaisesMessage(ValueError, "xrobot is not pinned; add `xrobot: <version>`"):
            self.check(None, "refs/heads/dev")
        self.check(None, "refs/heads/feature/board")

    def test_commit_pins_must_be_on_the_target_line(self):
        self.check(self.dev_only, "refs/heads/dev")
        self.check(self.merged, "refs/heads/master")
        with self.assertRaisesMessage(
            ValueError,
            f"xrobot pin {self.dev_only[:12]} is not on the tool's master line; pin a release "
            "version or a merged commit",
        ):
            self.check(self.dev_only, "refs/heads/master")
        with self.assertRaisesMessage(
            ValueError,
            f"xrobot pin {self.feature[:12]} is not on the tool's dev line; pin a release "
            "version or a merged commit",
        ):
            self.check(self.feature, "refs/heads/dev")

    def test_commit_pins_of_both_tools_cannot_be_checked_offline(self):
        # generator 的锁定写在 User/libxr_config.yaml 中。
        # The generator pin is written in User/libxr_config.yaml.
        generator = "0123456789abcdef0123456789abcdef01234567"
        with self.assertRaisesMessage(
            ValueError,
            f"xrobot pin {self.merged[:12]} cannot be checked offline\n"
            "generator pin 0123456789ab cannot be checked offline",
        ):
            self.check(self.merged, "refs/heads/master", offline=True, generator=generator)


class ModulesYaml(UpstreamTestCase):
    """读取 modules.yaml。
    Reading modules.yaml.
    """

    def test_only_modules_and_the_tool_pin_are_allowed(self):
        path = self.write("Modules/modules.yaml", "xrobot: 1.0.0\nmodules: [team/A@dev]\n")
        self.assertEqual(read_modules_yaml(path), ([{"id": "team/A", "ref": "dev"}], "1.0.0"))
        self.write("Modules/modules.yaml", "modules: []\nlock: true\n")
        with self.assertRaisesMessage(
            ValueError, f"{path}: unknown key(s) lock; allowed: modules, xrobot"
        ):
            read_modules_yaml(path)

    def test_the_tool_pin_is_a_release_version_or_a_full_commit(self):
        path = self.root / "Modules/modules.yaml"
        for pin in ("1.0.0", "1.2.3-beta.1", "a" * 40):
            with self.subTest(pin=pin):
                self.write(path, f'xrobot: "{pin}"\nmodules: []\n')
                self.assertEqual(read_modules_yaml(path)[1], pin)
        for pin in ("master", "1.0", "A" * 40, "abc123"):
            with (
                self.subTest(pin=pin),
                self.assertRaisesMessage(
                    ValueError,
                    f"{path}: xrobot must be a release version (e.g. 1.0.0) or a 40-hex commit",
                ),
            ):
                self.write(path, f'xrobot: "{pin}"\nmodules: []\n')
                read_modules_yaml(path)
        self.write(path, "modules: []\n")
        self.assertEqual(read_modules_yaml(path), ([], None))

    def test_requests_are_canonical_ids(self):
        path = self.root / "Modules/modules.yaml"
        self.write(
            path,
            "modules:\n  - team/A@same-or-dev\n  - {id: team/B, ref: v1, context_ref: refs/heads/dev}\n",
        )
        self.assertEqual(
            read_modules_yaml(path)[0],
            [
                {"id": "team/A", "ref": "same-or-dev"},
                {"id": "team/B", "ref": "v1", "context_ref": "refs/heads/dev"},
            ],
        )
        for request in ("A", "team/A@-x", "{id: team/A, branch: dev}"):
            with self.subTest(request=request), self.assertRaises(ValueError):
                self.write(path, f"modules:\n  - {request}\n")
                read_modules_yaml(path)


class RepositoryIdentity(TestCase):
    """判断两个地址是否指向同一个仓库。
    Whether two locations name the same repository.
    """

    def test_urls_are_normalized(self):
        same = [
            "https://github.com/Team/Repo.git",
            "git@github.com:team/repo",
            "https://GitHub.com/team/repo/",
            "ssh://git@github.com/team/Repo.git",
        ]
        for url in same:
            with self.subTest(url=url):
                self.assertEqual(repository_identity(url), "github.com/team/repo")
        self.assertEqual(
            repository_identity("https://gitlab.example.com/Team/Repo.git"),
            "gitlab.example.com/Team/Repo",
        )
        self.assertNotEqual(
            repository_identity("https://github.com/team/repo"),
            repository_identity("https://github.com/other/repo"),
        )
        self.assertTrue(same_repository("https://github.com/a/b", "git@github.com:A/B.git"))


if __name__ == "__main__":
    unittest.main()
