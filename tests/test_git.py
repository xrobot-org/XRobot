"""读取检出状态的 git 查询（xrobot.git）。
The git queries that read checkout state (xrobot.git).
"""

from unittest import mock

from fixtures import TempDirTestCase, run_git

from xrobot.git import checkout_state, head_commit, origin_url


class GitQueries(TempDirTestCase):
    """HEAD、检出状态和 origin 地址的查询。
    Queries of HEAD, checkout state and the origin URL.
    """

    def setUp(self):
        super().setUp()
        self.repo = self.tmp / "repo"
        self.repo.mkdir()
        run_git(self.repo, "init", "-q", "-b", "main")
        (self.repo / "a.txt").write_text("a\n", encoding="utf-8")
        run_git(self.repo, "add", "-A")
        run_git(self.repo, "commit", "-q", "-m", "one")
        self.commit = run_git(self.repo, "rev-parse", "HEAD")

    def test_head_commit_reads_a_detached_head_and_asks_git_on_a_branch(self):
        # 在 main 分支上：由 git 回答。
        # On branch main: git answers.
        self.assertEqual(head_commit(self.repo), self.commit)
        run_git(self.repo, "checkout", "-q", "--detach")
        with mock.patch("xrobot.git.subprocess.run") as run:
            self.assertEqual(head_commit(self.repo), self.commit)
        run.assert_not_called()
        empty = self.tmp / "empty"
        empty.mkdir()
        run_git(empty, "init", "-q")
        self.assertIsNone(head_commit(empty))

    def test_checkout_state_reports_commit_branch_and_changes(self):
        self.assertEqual(checkout_state(self.repo), (self.commit, "main", False))
        (self.repo / "b.txt").write_text("new\n", encoding="utf-8")
        self.assertEqual(checkout_state(self.repo), (self.commit, "main", True))
        (self.repo / "b.txt").unlink()
        run_git(self.repo, "checkout", "-q", "--detach")
        self.assertEqual(checkout_state(self.repo), (self.commit, None, False))

    def test_origin_url_reads_git_config_and_asks_git_otherwise(self):
        run_git(self.repo, "remote", "add", "origin", "https://example.com/team/A.git")
        with mock.patch("xrobot.git.subprocess.run") as run:
            self.assertEqual(origin_url(self.repo), "https://example.com/team/A.git")
        run.assert_not_called()
        # 带引号的值由 git 解析。
        # git parses a quoted value.
        config = self.repo / ".git" / "config"
        text = config.read_text(encoding="utf-8")
        config.write_text(
            text.replace("https://example.com/team/A.git", '"D:/Mirror Folder/A"'),
            encoding="utf-8",
        )
        self.assertEqual(origin_url(self.repo), "D:/Mirror Folder/A")
        # 工作树的 .git 是文件，由 git 回答。
        # A worktree's .git is a file, so git answers.
        run_git(self.repo, "worktree", "add", "-q", "--detach", str(self.tmp / "tree"))
        self.assertTrue((self.tmp / "tree" / ".git").is_file())
        self.assertEqual(origin_url(self.tmp / "tree"), "D:/Mirror Folder/A")
