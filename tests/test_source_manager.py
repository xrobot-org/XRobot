"""源和 index（xrobot.source_manager）。
Sources and indexes (xrobot.source_manager).
"""

from fixtures import UpstreamTestCase

from xrobot.source_manager import (
    SourceManager,
    add_index_entry,
    add_source,
    load_yaml,
    validate_id,
)


class Sources(UpstreamTestCase):
    """读取 sources.yaml 和 index.yaml。
    Reading sources.yaml and index.yaml.
    """

    def test_validation_labels_are_bound_to_tested_versions(self):
        self.upstream("team/A")
        self.entries[0]["status"] = "official"
        self.write_yaml(self.index, {"packages": self.entries})
        with self.assertRaisesMessage(
            ValueError, f"{self.index}: team/A: status official needs tested_ref and tested_libxr"
        ):
            SourceManager(self.modules / "sources.yaml")
        self.entries[0].update(tested_ref="2026-09-15", tested_libxr="6.0.0")
        self.write_yaml(self.index, {"packages": self.entries})
        self.assertEqual(
            SourceManager(self.modules / "sources.yaml").packages["team/A"]["tested_ref"],
            "2026-09-15",
        )

    def test_unquoted_date_tags_are_kept_as_text(self):
        self.upstream("team/A")
        self.entries[0].update(status="verified", tested_ref="2026-09-15", tested_libxr="6.0.0")
        self.write_yaml(self.index, {"packages": self.entries})
        self.index.write_text(
            self.index.read_text(encoding="utf-8").replace("'2026-09-15'", "2026-09-15"),
            encoding="utf-8",
        )
        self.assertEqual(
            SourceManager(self.modules / "sources.yaml").packages["team/A"]["tested_ref"],
            "2026-09-15",
        )

    def test_bsp_entries_are_discovery_metadata_only(self):
        self.write_yaml(self.index, {"bsps": ["https://github.com/team/board.git"]})
        manager = SourceManager(self.modules / "sources.yaml")
        record = manager.packages["team/board"]
        self.assertEqual(set(record), {"id", "type", "repo", "source", "canonical"})
        self.assertEqual(manager.resolve_id("board", "bsp"), "team/board")

    def test_equal_priority_sources_must_agree(self):
        self.upstream("team/A")
        other = self.write_yaml(
            self.tmp / "other.yaml",
            {"packages": [{"id": "team/A", "type": "module", "repo": "https://x.invalid/A"}]},
        )
        self.write_yaml(
            self.modules / "sources.yaml",
            {"sources": [{"url": str(self.index)}, {"url": str(other)}]},
        )
        with self.assertRaisesMessage(
            ValueError,
            f"team/A: {self.index} and {other} have the same priority 0 but list different "
            f"repositories ({self.tmp / 'upstream/team/A'}, https://x.invalid/A); give one of "
            "them another priority in sources.yaml",
        ):
            SourceManager(self.modules / "sources.yaml")

    def test_short_names_ignore_case(self):
        self.upstream("team/A")
        self.assertEqual(SourceManager(self.modules / "sources.yaml").resolve_id("a"), "team/A")

    def test_an_id_spelled_with_the_index_namespace_names_the_new_id(self):
        # XRobot 1.0 以前的 id 用 index 的 namespace（qdu-future/CMD），1.0 用 GitHub 的 owner；
        # 以前只报“找不到包”，看不出应该改成什么。
        # Before XRobot 1.0 an id used the namespace of the index (qdu-future/CMD), and 1.0
        # uses the GitHub owner; only "Package not found" used to be reported, without the
        # id to write instead.
        self.write_yaml(
            self.index,
            {"namespace": "qdu-future", "modules": ["https://github.com/QDU-Robomaster/CMD.git"]},
        )
        manager = SourceManager(self.modules / "sources.yaml")
        with self.assertRaisesMessage(
            ValueError,
            "Package not found: qdu-future/CMD; XRobot 1.0 names packages by the owner of their "
            "GitHub repository, write QDU-Robomaster/CMD",
        ):
            manager.resolve_id("qdu-future/CMD")
        with self.assertRaisesMessage(ValueError, "Package not found: qdu-future/Missing"):
            manager.resolve_id("qdu-future/Missing")
        with self.assertRaisesMessage(ValueError, "Package not found: qdu-future/CMD"):
            manager.resolve_id("qdu-future/CMD", "bsp")
        self.write_yaml(
            self.index,
            {
                "mirror_of": "old-team",
                "modules": [{"id": "team/Cmd", "repo": "https://git.example.com/m/Cmd.git"}],
            },
        )
        with self.assertRaisesMessage(
            ValueError,
            "Package not found: old-team/cmd; XRobot 1.0 names packages by the owner of their "
            "GitHub repository, write team/Cmd",
        ):
            SourceManager(self.modules / "sources.yaml").resolve_id("old-team/cmd")

    def test_errors_name_the_index_and_the_entry(self):
        cases = (
            (
                {"modules": ["https://git.example.com/x/Filter.git"]},
                "https://git.example.com/x/Filter.git: cannot derive owner/Repo; add "
                "`id: owner/Repo` to the entry or `namespace:` to the index",
            ),
            (
                {"modules": [{"repo": "https://github.com/team/A.git", "type": "bsp"}]},
                "https://github.com/team/A.git: type must be module",
            ),
            ({"modules": [{"id": "team/A"}]}, "team/A: missing repo URL"),
            (
                {"modules": [{"id": "team/A", "repo": "a", "status": "gold"}]},
                "team/A: unknown status gold; use community, verified, official",
            ),
            (
                {"modules": ["https://github.com/team/A.git", "https://github.com/Team/a"]},
                "Team/a is listed more than once",
            ),
            ({"modules": [{"id": "A", "repo": "a"}]}, "A: Expected canonical owner/repo: 'A'"),
        )
        for data, message in cases:
            with self.subTest(message=message):
                self.write_yaml(self.index, data)
                with self.assertRaisesMessage(ValueError, f"{self.index}: {message}"):
                    SourceManager(self.modules / "sources.yaml")
        self.write_yaml(self.modules / "sources.yaml", {"sources": [{"priority": 0}]})
        with self.assertRaisesMessage(
            ValueError, f"{self.modules / 'sources.yaml'}: every source needs a url"
        ):
            SourceManager(self.modules / "sources.yaml")

    def test_a_mirror_supplies_the_fetch_url_whatever_its_priority(self):
        self.upstream("team/A")
        mirror = self.write_yaml(
            self.tmp / "mirror.yaml", {"mirror_of": "team", "modules": ["https://m.example/A.git"]}
        )
        for priority in (-1, 0, 1):
            with self.subTest(priority=priority):
                self.write_yaml(
                    self.modules / "sources.yaml",
                    {
                        "sources": [
                            {"url": str(self.index)},
                            {"url": str(mirror), "priority": priority},
                        ]
                    },
                )
                record = SourceManager(self.modules / "sources.yaml").packages["team/A"]
                self.assertEqual(
                    (record["repo"], record["canonical"], record["source"]),
                    ("https://m.example/A.git", str(self.tmp / "upstream/team/A"), str(self.index)),
                )
                self.assertEqual(list(record)[:5], ["id", "type", "repo", "canonical", "source"])

    def test_adding_keeps_comments_layout_and_line_endings(self):
        sources = self.modules / "sources.yaml"
        self.write(
            sources,
            "# team sources\r\nsources:\r\n  - url: a.yaml  # ours\r\n    priority: 0\r\n",
        )
        self.assertTrue(add_source(sources, "https://example.com/index.yaml", 2))
        self.assertFalse(add_source(sources, "a.yaml"))
        self.assertEqual(
            sources.read_bytes(),
            b"# team sources\r\nsources:\r\n  - url: a.yaml  # ours\r\n    priority: 0\r\n"
            b"  - url: https://example.com/index.yaml\r\n    priority: 2\r\n",
        )
        index = self.tmp / "local.yaml"
        self.write(index, "namespace: me  # team\nmodules: []\nbsps: []\n")
        self.assertTrue(add_index_entry(index, "https://git.example.com/me/A.git"))
        self.assertFalse(add_index_entry(index, "https://git.example.com/me/A.git"))
        self.assertEqual(
            self.read(index),
            "namespace: me  # team\nmodules:\n  - https://git.example.com/me/A.git\nbsps: []\n",
        )
        fresh = self.tmp / "fresh.yaml"
        add_source(fresh, "a.yaml")
        self.assertEqual(load_yaml(fresh), {"sources": [{"url": "a.yaml", "priority": 0}]})
        self.write(index, "modules: [a, b]\n")
        with self.assertRaisesMessage(
            ValueError, f"{index}: write modules as a block list (one '- ' item per line) first"
        ):
            add_index_entry(index, "c")

    def test_traversal_identities_are_rejected(self):
        for identity in (
            "../../out",
            "team/../out",
            "team/.",
            "team/..",
            "team/-bad",
            "/tmp/evil",
            "A",
        ):
            with self.subTest(identity=identity), self.assertRaises(ValueError):
                validate_id(identity)
        self.assertEqual(validate_id("team/A.b-c_1"), "team/A.b-c_1")
