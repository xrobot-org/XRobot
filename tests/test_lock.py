"""Module resolution and xrobot.lock (xrobot.init_module), locked discovery (xrobot.module_parser)
and source catalogs (xrobot.source_manager)."""
import os
import unittest

import yaml
from fixtures import BspTestCase, UpstreamTestCase, manifest_block, run_git

from xrobot.init_module import read_modules_yaml, repository_identity, same_repository, write_cmake
from xrobot.module_parser import discover_modules, manifest_from_text, select_module
from xrobot.source_manager import SourceManager, validate_id


class Resolution(UpstreamTestCase):
    def test_dependencies_resolve_to_exact_commits_and_a_module_list(self):
        b = self.upstream('team/B')
        self.upstream('team/A', ['team/B@master'])
        self.configure(['team/A@master'])
        lock = self.sync()
        self.assertEqual(set(lock['modules']), {'team/A', 'team/B'})
        self.assertEqual(lock['version'], 1)
        self.assertEqual(lock['requests'], [{'id': 'team/A', 'ref': 'master'}])
        self.assertEqual(lock['modules']['team/B']['commit'], run_git(b, 'rev-parse', 'HEAD'))
        self.assertEqual(self.head('team/B'), lock['modules']['team/B']['commit'])
        self.assertEqual(yaml.safe_load(self.lock_bytes()), lock)
        cmake = (self.modules / 'CMakeLists.txt').read_text(encoding='utf-8')
        self.assertIn('include("${CMAKE_CURRENT_LIST_DIR}/team/A/CMakeLists.txt")', cmake)
        self.assertIn('include("${CMAKE_CURRENT_LIST_DIR}/team/B/CMakeLists.txt")', cmake)
        self.assertFalse((self.modules / 'team/A/xrobot.lock').exists())

    def test_the_default_branch_is_used_without_a_ref(self):
        self.upstream('team/A')
        self.configure(['team/A'])
        row = self.sync()['modules']['team/A']
        self.assertEqual((row['ref_kind'], row['resolved_ref']), ('branch', 'master'))

    def test_the_lock_is_stable_and_updates_are_explicit(self):
        a = self.upstream('team/A')
        self.configure(['team/A@master'])
        old = self.sync()['modules']['team/A']['commit']
        new = self.commit(a, [], 'change')
        self.assertEqual(self.sync()['modules']['team/A']['commit'], old)
        self.assertEqual(self.sync(frozen=True)['modules']['team/A']['commit'], old)
        self.assertEqual(self.sync(update=[])['modules']['team/A']['commit'], new)
        self.assertEqual(self.head('team/A'), new)

    def test_frozen_restores_the_lock_without_writing_it(self):
        a = self.upstream('team/A')
        self.configure(['team/A'])
        locked = self.sync()['modules']['team/A']['commit']
        self.commit(a, [], 'change')
        path = self.root / 'xrobot.lock'
        path.write_bytes(b'# reviewed\n' + self.lock_bytes())
        before = path.read_bytes()
        run_git(self.modules / 'team/A', 'fetch', '-q', 'origin')
        run_git(self.modules / 'team/A', 'checkout', '-q', '--detach', 'origin/master')
        self.assertEqual(self.sync(frozen=True)['modules']['team/A']['commit'], locked)
        self.assertEqual(self.head('team/A'), locked)
        self.assertEqual(path.read_bytes(), before)

    def test_frozen_requires_a_lock_that_matches_modules_yaml(self):
        self.upstream('team/A')
        self.upstream('team/C')
        self.configure(['team/A'])
        with self.assertRaisesRegex(ValueError, 'xrobot.lock does not exist; run `xrobot setup` once without --frozen'):
            self.sync(frozen=True)
        self.sync()
        before = self.lock_bytes()
        self.configure(['team/A', 'team/C'])
        with self.assertRaisesRegex(ValueError, r'Modules/modules.yaml differs from xrobot.lock \(\+team/C\)'):
            self.sync(frozen=True)
        self.configure(['team/A@dev'])
        with self.assertRaisesRegex(ValueError, r'differs from xrobot.lock \(~team/A\)'):
            self.sync(frozen=True)
        self.assertEqual(self.lock_bytes(), before)

    def test_update_cannot_be_combined_with_frozen_or_offline(self):
        self.configure([])
        for flags in ({'frozen': True}, {'offline': True}):
            with self.subTest(flags=flags), self.assertRaisesRegex(ValueError, '--update cannot be combined'):
                self.sync(update=[], **flags)

    def test_offline_needs_neither_catalog_nor_remote(self):
        a = self.upstream('team/A')
        self.configure(['team/A'])
        old = self.sync()['modules']['team/A']['commit']
        self.index.unlink()
        (self.modules / 'sources.yaml').unlink()
        a.rename(a.with_name('unavailable'))
        self.assertEqual(self.sync(offline=True)['modules']['team/A']['commit'], old)

    def test_requests_use_canonical_ids_and_short_names_must_be_unambiguous(self):
        self.upstream('first/A')
        self.upstream('second/A')
        self.configure(['A'])
        with self.assertRaisesRegex(ValueError, "Expected canonical owner/repo: 'A'"):
            self.sync()
        manager = SourceManager(self.modules / 'sources.yaml')
        with self.assertRaisesRegex(ValueError, 'Ambiguous package A; specify first/A, second/A'):
            manager.resolve_id('A')
        self.assertEqual(manager.resolve_id('first/a'), 'first/A')
        self.configure(['first/A'])
        self.assertEqual(set(self.sync()['modules']), {'first/A'})

    def test_bsp_catalog_entries_are_not_dependencies(self):
        self.upstream('team/Board', kind='bsp')
        self.configure(['team/Board'])
        with self.assertRaisesRegex(ValueError, 'BSP catalog entry, not a Module dependency'):
            self.sync()
        self.assertFalse((self.modules / 'team/Board').exists())

    def test_dependency_cycles_are_reported(self):
        self.upstream('team/A', ['team/B'])
        self.upstream('team/B', ['team/A'])
        self.configure(['team/A'])
        with self.assertRaisesRegex(ValueError, 'Package dependency cycle: team/A -> team/B -> team/A'):
            self.sync()

    def test_a_ref_that_is_both_branch_and_tag_must_be_qualified(self):
        a = self.upstream('team/A')
        run_git(a, 'tag', 'dev')
        self.configure(['team/A@dev'])
        with self.assertRaisesRegex(ValueError, 'Ref exists as branch and tag'):
            self.sync()
        self.configure(['team/A@refs/heads/dev'])
        self.assertEqual(self.sync()['modules']['team/A']['ref_kind'], 'branch')

    def test_unknown_refs_are_reported(self):
        self.upstream('team/A')
        self.configure(['team/A@missing'])
        with self.assertRaisesRegex(ValueError, 'Unknown dependency ref: missing'):
            self.sync()

    def test_conflicting_dependency_commits_leave_lock_and_checkouts_unchanged(self):
        b = self.upstream('team/B')
        run_git(b, 'tag', 'old')
        old = run_git(b, 'rev-parse', 'HEAD')
        self.commit(b, [], 'new')
        run_git(b, 'tag', 'new')
        self.upstream('team/A', ['team/B@old'])
        self.upstream('team/C', ['team/B@new'])
        self.configure(['team/A'])
        self.sync()
        before = self.lock_bytes()
        self.configure(['team/A', 'team/C'])
        with self.assertRaisesRegex(ValueError, 'Dependency conflict for team/B'):
            self.sync(update=[])
        self.assertEqual(self.lock_bytes(), before)
        self.assertEqual(self.head('team/B'), old)


class MinimalChange(UpstreamTestCase):
    def setUp(self):
        super().setUp()
        self.b = self.upstream('team/B')
        self.a = self.upstream('team/A', ['team/B'])
        self.configure(['team/A'])
        self.first = self.sync()['modules']
        self.moved_a = self.commit(self.a, ['team/B'], 'a moves')
        self.moved_b = self.commit(self.b, [], 'b moves')

    def test_adding_a_request_resolves_only_the_new_module(self):
        c = self.upstream('team/C')
        self.configure(['team/A', 'team/C'])
        lock = self.sync()['modules']
        self.assertEqual(lock['team/A']['commit'], self.first['team/A']['commit'])
        self.assertEqual(lock['team/B']['commit'], self.first['team/B']['commit'])
        self.assertEqual(lock['team/C']['commit'], run_git(c, 'rev-parse', 'HEAD'))

    def test_removing_a_request_drops_only_its_modules(self):
        self.upstream('team/C')
        self.configure(['team/A', 'team/C'])
        self.sync()
        self.configure(['team/C'])
        lock = self.sync()
        self.assertEqual(set(lock['modules']), {'team/C'})
        self.assertIn('team/C', (self.modules / 'CMakeLists.txt').read_text(encoding='utf-8'))
        self.assertNotIn('team/A', (self.modules / 'CMakeLists.txt').read_text(encoding='utf-8'))

    def test_update_of_named_modules_moves_only_them(self):
        lock = self.sync(update=['team/B'])['modules']
        self.assertEqual(lock['team/B']['commit'], self.moved_b)
        self.assertEqual(lock['team/A']['commit'], self.first['team/A']['commit'])
        lock = self.sync(update=['A'])['modules']
        self.assertEqual(lock['team/A']['commit'], self.moved_a)

    def test_update_without_names_moves_everything(self):
        lock = self.sync(update=[])['modules']
        self.assertEqual((lock['team/A']['commit'], lock['team/B']['commit']), (self.moved_a, self.moved_b))

    def test_update_takes_module_ids_from_the_lock(self):
        with self.assertRaisesRegex(ValueError, 'team/Z is not in xrobot.lock'):
            self.sync(update=['team/Z'])

    def test_a_new_module_requiring_a_locked_module_elsewhere_suggests_update(self):
        run_git(self.b, 'tag', 'v2')
        self.upstream('team/C', ['team/B@v2'])
        before = self.lock_bytes()
        self.configure(['team/A', 'team/C'])
        with self.assertRaisesRegex(ValueError, r'team/C requires team/B at v2, but xrobot\.lock keeps %s; run '
                                                r'`xrobot setup --update team/B`' % self.first['team/B']['commit'][:12]):
            self.sync()
        self.assertEqual(self.lock_bytes(), before)
        lock = self.sync(update=['team/B'])['modules']
        self.assertEqual(lock['team/B']['commit'], self.moved_b)

    def test_changing_the_request_of_a_locked_module_re_resolves_it(self):
        run_git(self.a, 'tag', 'v1', self.first['team/A']['commit'])
        self.configure(['team/A@master'])
        self.assertEqual(self.sync()['modules']['team/A']['commit'], self.moved_a)
        self.configure(['team/A@v1'])
        self.assertEqual(self.sync()['modules']['team/A']['commit'], self.first['team/A']['commit'])


class LocalWork(UpstreamTestCase):
    def setUp(self):
        super().setUp()
        self.a = self.upstream('team/A')
        self.configure(['team/A'])
        self.locked = self.sync()['modules']['team/A']['commit']
        self.commit(self.a, [], 'upstream moves')

    def test_uncommitted_changes_are_never_overwritten(self):
        header = self.modules / 'team/A/A.hpp'
        header.write_bytes(header.read_bytes() + b'\n// local work\n')
        before = header.read_bytes()
        with self.assertRaisesRegex(ValueError, 'team/A has uncommitted changes; they are kept'):
            self.sync(update=[])
        self.assertEqual(header.read_bytes(), before)

    def test_an_unpushed_local_commit_is_not_moved(self):
        folder = self.modules / 'team/A'
        run_git(folder, 'checkout', '-q', '-b', 'work')
        (folder / 'A.hpp').write_bytes((folder / 'A.hpp').read_bytes() + b'\n// local\n')
        run_git(folder, 'commit', '-q', '-am', 'local')
        local = run_git(folder, 'rev-parse', 'HEAD')
        before = self.lock_bytes()
        for flags in ({'update': []}, {'frozen': True}):
            with self.subTest(flags=flags), self.assertRaisesRegex(
                    ValueError, r'team/A is at local commit %s that is not on any remote branch or tag.*push them '
                                r'to a branch of the module and run `xrobot setup --update team/A`' % local[:12]):
                self.sync(**flags)
            self.assertEqual(self.head('team/A'), local)
        self.assertEqual(self.lock_bytes(), before)

    def test_generation_rejects_a_checkout_away_from_the_lock_with_the_fix(self):
        from xrobot.project import Project
        folder = self.modules / 'team/A'
        run_git(folder, 'fetch', '-q', 'origin')
        run_git(folder, 'checkout', '-q', '--detach', 'origin/master')
        with self.assertRaisesRegex(ValueError, r'team/A is checked out at \w{12} but xrobot.lock pins %s.*'
                                                r'`xrobot setup --update team/A`.*`xrobot setup --frozen`'
                                    % self.locked[:12]):
            discover_modules(self.modules, Project(self.root).lock)


class Contexts(UpstreamTestCase):
    def test_same_or_dev_follows_a_stacked_branch_then_falls_back_to_dev(self):
        b = self.upstream('team/B')
        a = self.upstream('team/A', [{'id': 'team/B', 'ref': 'same-or-dev'}])
        run_git(b, 'checkout', '-q', '-b', 'feature/next')
        feature = self.commit(b, [], 'feature')
        run_git(a, 'checkout', '-q', '-b', 'feature/next')
        self.configure(['team/A@feature/next'])
        first = self.sync()
        self.assertEqual(first['modules']['team/B']['commit'], feature)
        self.assertEqual(first['modules']['team/B']['resolved_ref'], 'feature/next')
        run_git(b, 'checkout', '-q', 'dev')
        run_git(b, 'merge', '-q', '--ff-only', 'feature/next')
        run_git(b, 'branch', '-q', '-D', 'feature/next')
        second = self.sync(update=[])
        self.assertEqual(second['modules']['team/B']['commit'], feature)
        self.assertEqual(second['modules']['team/B']['resolved_ref'], 'dev')
        self.assertEqual(first['modules']['team/A']['commit'], second['modules']['team/A']['commit'])

    def test_a_tag_never_falls_back_to_dev(self):
        b = self.upstream('team/B')
        a = self.upstream('team/A', [{'id': 'team/B', 'ref': 'same-or-dev'}])
        run_git(a, 'tag', '2026-09-15')
        self.configure(['team/A@2026-09-15'])
        with self.assertRaisesRegex(ValueError, 'Required same tag is missing: 2026-09-15'):
            self.sync()
        self.assertFalse((self.root / 'xrobot.lock').exists())
        run_git(b, 'tag', '2026-09-15')
        row = self.sync()['modules']['team/B']
        self.assertEqual((row['ref_kind'], row['resolved_ref']), ('tag', '2026-09-15'))

    def test_a_detached_commit_request_needs_a_logical_context_for_its_dependencies(self):
        self.upstream('team/B')
        a = self.upstream('team/A', [{'id': 'team/B', 'ref': 'same-or-dev'}])
        sha = run_git(a, 'rev-parse', 'HEAD')
        self.configure(['team/A@' + sha])
        with self.assertRaisesRegex(ValueError, 'same-or-dev requires a BSP branch/tag context'):
            self.sync()
        self.configure([{'id': 'team/A', 'ref': sha, 'context_ref': 'refs/heads/pr-feature'}])
        self.assertEqual(self.sync()['modules']['team/B']['resolved_ref'], 'dev')

    def test_root_same_or_dev_uses_the_bsp_branch_only_while_resolving(self):
        a = self.upstream('team/A')
        run_git(a, 'branch', 'feature/board')
        run_git(self.root, 'init', '-q', '-b', 'feature/board')
        self.configure(['team/A@same-or-dev'])
        first = self.sync()
        self.assertEqual(first['modules']['team/A']['resolved_ref'], 'feature/board')
        self.assertNotIn('context', first['modules']['team/A'])
        self.assertEqual(self.sync(offline=True), first)
        run_git(self.root, 'symbolic-ref', 'HEAD', 'refs/heads/other')
        self.assertEqual(self.sync(frozen=True), first)
        self.assertEqual(self.sync(update=[])['modules']['team/A']['resolved_ref'], 'dev')

    def test_a_tag_context_is_exact(self):
        a = self.upstream('team/A')
        self.configure(['team/A@same-or-dev'])
        with self.assertRaisesRegex(ValueError, 'Required same tag is missing: release-check'):
            self.sync(context_ref='refs/tags/release-check')
        run_git(a, 'tag', 'release-check')
        first = self.sync(context_ref='refs/tags/release-check')
        self.assertEqual(first['modules']['team/A']['ref_kind'], 'tag')
        self.assertEqual(self.sync(context_ref='refs/tags/release-check', offline=True), first)

    def test_same_or_dev_without_a_bsp_branch_needs_a_context_ref(self):
        self.upstream('team/A')
        self.configure(['team/A@same-or-dev'])
        with self.assertRaisesRegex(ValueError, 'same/same-or-dev requests need a BSP branch; pass --context-ref'):
            self.sync()
        self.assertEqual(self.sync(context_ref='refs/heads/review')['modules']['team/A']['resolved_ref'], 'dev')

    def test_a_request_can_carry_its_own_context(self):
        self.upstream('team/A')
        self.configure([{'id': 'team/A', 'ref': 'same', 'context_ref': 'refs/heads/dev'}])
        first = self.sync()
        self.assertEqual(first['modules']['team/A']['resolved_ref'], 'dev')
        self.assertEqual(self.sync(offline=True), first)

    def test_context_refs_must_be_qualified(self):
        self.upstream('team/A')
        self.configure(['team/A@same-or-dev'])
        with self.assertRaisesRegex(ValueError, 'must start with refs/heads/ or refs/tags/'):
            self.sync(context_ref='dev')


class LockFile(UpstreamTestCase):
    def test_relative_catalog_and_local_sources_are_stored_relative_to_the_lock(self):
        self.upstream('team/A')
        self.entries[0]['repo'] = 'upstream/team/A'
        self.write_yaml(self.index, {'packages': self.entries})
        self.write_yaml(self.modules / 'sources.yaml', {'sources': [{'url': '../../index.yaml'}]})
        self.configure(['team/A'])
        first = self.sync()
        row = first['modules']['team/A']
        self.assertEqual((row['repo'], row['source']), ('../upstream/team/A', '../index.yaml'))
        self.assertNotIn(str(self.tmp), self.lock_bytes().decode('utf-8'))
        self.assertNotIn(self.tmp.as_posix(), self.lock_bytes().decode('utf-8'))
        self.assertEqual(self.sync(offline=True), first)

    def test_a_locked_local_source_stays_relative_to_the_lock_when_run_from_a_subdirectory(self):
        self.upstream('team/A')
        self.entries[0]['repo'] = 'upstream/team/A'
        self.write_yaml(self.index, {'packages': self.entries})
        self.configure(['team/A'])
        self.sync()
        self.upstream('team/B')
        self.configure(['team/A', 'team/B'])
        (self.root / 'User').mkdir()
        lock = self.sync(cwd=self.root / 'User')
        self.assertEqual(lock['modules']['team/A']['repo'], '../upstream/team/A')

    def test_file_urls_are_stored_without_machine_paths(self):
        a = self.upstream('team/A')
        self.entries[0]['repo'] = a.as_uri()
        self.write_yaml(self.index, {'packages': self.entries})
        self.configure(['team/A@dev'])
        first = self.sync()
        self.assertEqual(first['modules']['team/A']['repo'], '../upstream/team/A')
        self.assertNotIn('file://', self.lock_bytes().decode('utf-8'))
        self.assertEqual(self.sync(offline=True), first)

    def test_the_lock_records_the_canonical_repository_and_fetches_from_a_mirror(self):
        a = self.upstream('team/A')
        mirror = self.tmp / 'mirror' / 'A'
        run_git(None, 'clone', '-q', '--bare', str(a), str(mirror))
        mirror_index = self.write_yaml(self.tmp / 'mirror.yaml', {'mirror_of': 'team', 'modules': [mirror.as_uri()]})
        self.write_yaml(self.modules / 'sources.yaml', {'sources': [
            {'url': str(self.index), 'priority': 0}, {'url': str(mirror_index), 'priority': 1}]})
        self.configure(['team/A'])
        row = self.sync()['modules']['team/A']
        self.assertEqual(row['repo'], '../upstream/team/A')
        self.assertTrue(same_repository(run_git(self.modules / 'team/A', 'remote', 'get-url', 'origin'), str(mirror)))

    def test_an_incomplete_or_extended_lock_is_rejected(self):
        self.upstream('team/B')
        self.upstream('team/A', ['team/B'])
        self.configure(['team/A'])
        lock = self.sync()
        missing = dict(lock, modules={'team/A': lock['modules']['team/A']})
        self.write_yaml(self.root / 'xrobot.lock', missing)
        with self.assertRaisesRegex(ValueError, 'xrobot.lock is missing or ambiguous for team/B'):
            self.sync(offline=True)
        self.upstream('team/C')
        self.configure(['team/C'])
        self.sync(update=[])
        extended = yaml.safe_load(self.lock_bytes())
        extended['modules'].update(lock['modules'])
        self.write_yaml(self.root / 'xrobot.lock', extended)
        with self.assertRaisesRegex(ValueError, 'outside the declared dependency closure: team/A, team/B'):
            self.sync(offline=True)

    def test_legacy_and_unknown_lock_formats_are_rejected(self):
        self.upstream('team/A')
        self.configure(['team/A'])
        lock = self.sync()
        legacy = yaml.safe_load(self.lock_bytes())
        legacy['modules']['team/A']['directory'] = 'team/A'
        self.write_yaml(self.root / 'xrobot.lock', legacy)
        with self.assertRaisesRegex(ValueError, 'Legacy lock entry for team/A'):
            self.sync(frozen=True)
        self.write_yaml(self.root / 'xrobot.lock', dict(lock, version=2))
        with self.assertRaisesRegex(ValueError, 'unsupported lock format'):
            self.sync(frozen=True)
        broken = dict(lock)
        broken['modules'] = {'team/A': dict(lock['modules']['team/A'], commit='abc')}
        self.write_yaml(self.root / 'xrobot.lock', broken)
        with self.assertRaisesRegex(ValueError, 'Invalid locked commit for team/A'):
            self.sync(frozen=True)

    def test_module_list_includes_or_adds_the_folder(self):
        folder = self.modules / 'team/Plain'
        folder.mkdir(parents=True)
        (self.modules / 'team/WithCMake').mkdir(parents=True)
        self.write(self.modules / 'team/WithCMake/CMakeLists.txt', '')
        write_cmake(self.modules, {'team/Plain': {}, 'team/WithCMake': {}})
        text = (self.modules / 'CMakeLists.txt').read_text(encoding='utf-8')
        self.assertTrue(text.startswith('# Generated by `xrobot setup` from xrobot.lock; do not edit or commit.'))
        self.assertIn('target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}/team/Plain")', text)
        self.assertIn('include("${CMAKE_CURRENT_LIST_DIR}/team/WithCMake/CMakeLists.txt")', text)


class ReleaseGate(UpstreamTestCase):
    """--release-ref: locked commits must be on the Module line of the BSP target."""

    def setUp(self):
        super().setUp()
        self.a = self.upstream('team/A')

    def lock_feature(self):
        run_git(self.a, 'checkout', '-q', '-b', 'feature/x')
        feature = self.commit(self.a, [], 'feature work')
        run_git(self.a, 'checkout', '-q', 'master')
        self.configure(['team/A@feature/x'])
        self.assertEqual(self.sync()['modules']['team/A']['commit'], feature)
        return feature

    def merge(self, branch, *options):
        run_git(self.a, 'checkout', '-q', branch)
        run_git(self.a, 'merge', '-q', *options)
        run_git(self.a, 'checkout', '-q', 'master')

    def test_unmerged_feature_commits_are_refused_for_dev(self):
        feature = self.lock_feature()
        before = self.lock_bytes()
        with self.assertRaisesRegex(ValueError, r'team/A: locked commit %s is not on dev .*run `xrobot setup --update '
                                                r'team/A --context-ref refs/heads/dev`' % feature[:12]):
            self.sync(frozen=True, release_ref='refs/heads/dev')
        self.assertEqual(self.lock_bytes(), before)

    def test_feature_commits_merged_into_dev_pass_for_dev_but_not_for_master(self):
        feature = self.lock_feature()
        self.merge('dev', '--no-ff', '-m', 'merge feature', 'feature/x')
        self.assertEqual(self.sync(frozen=True, release_ref='refs/heads/dev')['modules']['team/A']['commit'], feature)
        for target in ('refs/heads/master', 'refs/heads/main', 'refs/tags/v1.0.0'):
            with self.subTest(target=target), self.assertRaisesRegex(ValueError, 'is not on master'):
                self.sync(frozen=True, release_ref=target)

    def test_squash_merged_feature_commits_are_reported(self):
        self.lock_feature()
        run_git(self.a, 'checkout', '-q', 'dev')
        run_git(self.a, 'merge', '-q', '--squash', 'feature/x')
        run_git(self.a, 'commit', '-q', '-m', 'squash feature')
        run_git(self.a, 'checkout', '-q', 'master')
        with self.assertRaisesRegex(ValueError, 'merged by squash/rebase'):
            self.sync(frozen=True, release_ref='refs/heads/dev')
        self.configure(['team/A@dev'])
        self.assertEqual(self.sync(release_ref='refs/heads/dev')['modules']['team/A']['resolved_ref'], 'dev')

    def test_master_hotfix_commits_are_refused_for_dev(self):
        hotfix = self.commit(self.a, [], 'hotfix')
        self.configure(['team/A@master'])
        self.assertEqual(self.sync(release_ref='refs/heads/master')['modules']['team/A']['commit'], hotfix)
        with self.assertRaisesRegex(ValueError, 'is not on dev'):
            self.sync(frozen=True, release_ref='refs/heads/dev')

    def test_other_targets_are_not_gated(self):
        self.lock_feature()
        self.sync(frozen=True, release_ref='refs/heads/feature/board')

    def test_an_explicitly_requested_tag_is_released(self):
        feature = self.lock_feature()
        run_git(self.a, 'tag', '2026-09-28', feature)
        self.configure(['team/A@2026-09-28'])
        self.assertEqual(self.sync(release_ref='refs/tags/v1.0.0')['modules']['team/A']['commit'], feature)

    def test_a_main_only_module_is_checked_against_main(self):
        main = self.upstream('team/M', branches=())
        run_git(main, 'branch', '-q', '-m', 'master', 'main')
        self.configure(['team/M@main'])
        self.assertEqual(self.sync(release_ref='refs/heads/master')['modules']['team/M']['ref_kind'], 'branch')

    def test_a_module_without_the_line_needs_a_commit_or_tag_pin(self):
        third = self.upstream('other/T', branches=())
        self.configure(['other/T@master'])
        with self.assertRaisesRegex(ValueError, 'other/T has no dev branch; request an explicit tag or commit'):
            self.sync(release_ref='refs/heads/dev')
        sha = run_git(third, 'rev-parse', 'HEAD')
        self.configure(['other/T@' + sha])
        self.assertEqual(self.sync(release_ref='refs/heads/dev')['modules']['other/T']['commit'], sha)


class ToolPins(UpstreamTestCase):
    """Tool pins follow the released-line rule of Modules."""

    def setUp(self):
        super().setUp()
        self.tool = self.upstream('xrobot-org/XRobot', catalog=False)
        self.merged = run_git(self.tool, 'rev-parse', 'HEAD')
        run_git(self.tool, 'checkout', '-q', 'dev')
        self.dev_only = self.commit(self.tool, [], 'dev work')
        run_git(self.tool, 'checkout', '-q', '-b', 'feature/x')
        self.feature = self.commit(self.tool, [], 'feature work')
        run_git(self.tool, 'checkout', '-q', 'master')
        self.cache = self.tmp / 'cache'
        run_git(None, 'clone', '-q', '--bare', str(self.tool), str(self.cache / 'xrobot.git'))

    def check(self, pin, target, offline=False, generator=None):
        from xrobot.init_module import check_tool_pins
        self.configure([], pin=pin)
        if generator is not None:
            self.write(self.root / 'User/libxr_config.yaml', 'generator: %s\n' % generator)
        check_tool_pins(self.project, target, offline, cache=self.cache)

    def test_release_versions_are_released(self):
        self.check('1.0.0', 'refs/heads/master')
        self.check('1.0.0', 'refs/tags/v1', generator='6.0.0')

    def test_missing_pins_are_errors_for_gated_targets(self):
        with self.assertRaisesRegex(ValueError, 'xrobot is not pinned; add `xrobot: <version>`'):
            self.check(None, 'refs/heads/dev')
        self.check(None, 'refs/heads/feature/board')

    def test_commit_pins_must_be_on_the_target_line(self):
        self.check(self.dev_only, 'refs/heads/dev')
        self.check(self.merged, 'refs/heads/master')
        with self.assertRaisesRegex(ValueError, "xrobot pin %s is not on the tool's master line" % self.dev_only[:12]):
            self.check(self.dev_only, 'refs/heads/master')
        with self.assertRaisesRegex(ValueError, "xrobot pin %s is not on the tool's dev line" % self.feature[:12]):
            self.check(self.feature, 'refs/heads/dev')

    def test_commit_pins_cannot_be_checked_offline(self):
        with self.assertRaisesRegex(ValueError, 'xrobot pin %s cannot be checked offline' % self.merged[:12]):
            self.check(self.merged, 'refs/heads/master', offline=True)


class ModulesYaml(UpstreamTestCase):
    def test_only_modules_and_the_tool_pin_are_allowed(self):
        path = self.write('Modules/modules.yaml', 'xrobot: 1.0.0\nmodules: [team/A@dev]\n')
        self.assertEqual(read_modules_yaml(path), ([{'id': 'team/A', 'ref': 'dev'}], '1.0.0'))
        self.write('Modules/modules.yaml', 'modules: []\nlock: true\n')
        with self.assertRaisesRegex(ValueError, r'unknown key\(s\) lock; allowed: modules, xrobot'):
            read_modules_yaml(path)

    def test_the_tool_pin_is_a_release_version_or_a_full_commit(self):
        path = self.root / 'Modules/modules.yaml'
        for pin in ('1.0.0', '1.2.3-beta.1', 'a' * 40):
            with self.subTest(pin=pin):
                self.write(path, 'xrobot: "%s"\nmodules: []\n' % pin)
                self.assertEqual(read_modules_yaml(path)[1], pin)
        for pin in ('master', '1.0', 'A' * 40, 'abc123'):
            with self.subTest(pin=pin), self.assertRaisesRegex(ValueError, 'xrobot must be a release version'):
                self.write(path, 'xrobot: "%s"\nmodules: []\n' % pin)
                read_modules_yaml(path)
        self.write(path, 'modules: []\n')
        self.assertEqual(read_modules_yaml(path), ([], None))

    def test_requests_are_canonical_ids(self):
        path = self.root / 'Modules/modules.yaml'
        self.write(path, 'modules:\n  - team/A@same-or-dev\n  - {id: team/B, ref: v1, context_ref: refs/heads/dev}\n')
        self.assertEqual(read_modules_yaml(path)[0], [
            {'id': 'team/A', 'ref': 'same-or-dev'}, {'id': 'team/B', 'ref': 'v1', 'context_ref': 'refs/heads/dev'}])
        for request in ('A', 'team/A@-x', '{id: team/A, branch: dev}'):
            with self.subTest(request=request), self.assertRaises(ValueError):
                self.write(path, 'modules:\n  - %s\n' % request)
                read_modules_yaml(path)


class RepositoryIdentity(unittest.TestCase):
    def test_urls_are_normalized(self):
        same = ['https://github.com/Team/Repo.git', 'git@github.com:team/repo', 'https://GitHub.com/team/repo/',
                'ssh://git@github.com/team/Repo.git']
        for url in same:
            with self.subTest(url=url):
                self.assertEqual(repository_identity(url), 'github.com/team/repo')
        self.assertEqual(repository_identity('https://gitlab.example.com/Team/Repo.git'), 'gitlab.example.com/Team/Repo')
        self.assertNotEqual(repository_identity('https://github.com/team/repo'),
                            repository_identity('https://github.com/other/repo'))
        self.assertTrue(same_repository('https://github.com/a/b', 'git@github.com:A/B.git'))


class LockedDiscovery(BspTestCase):
    def test_only_locked_modules_at_their_commits_are_loaded(self):
        self.module('A', 'class A { public: A() {} };')
        self.write('Modules/stale/B/B.hpp', 'class B { public: B() {} };\n')
        modules = discover_modules(self.root / 'Modules', self.root / 'xrobot.lock')
        self.assertEqual(set(modules), {'team/A'})
        self.assertEqual(modules['team/A']['name'], 'A')

    def test_a_lock_is_required(self):
        (self.root / 'xrobot.lock').unlink()
        with self.assertRaisesRegex(ValueError, 'xrobot.lock does not exist; run `xrobot setup`'):
            discover_modules(self.root / 'Modules', self.root / 'xrobot.lock')

    def test_every_lock_problem_is_reported_with_its_fix(self):
        a = self.module('A', 'class A { public: A() {} };')
        self.module('B', 'class B { public: B() {} };')
        self.module('C', 'class C { public: C() {} };')
        self.locked['team/A'] = '0' * 40
        self.locked['team/Gone'] = '1' * 40
        self.write_lock()
        import shutil
        shutil.rmtree(self.root / 'Modules/team/C/.git', onerror=lambda f, p, e: (os.chmod(p, 0o700), f(p)))
        with self.assertRaises(ValueError) as context:
            discover_modules(self.root / 'Modules', self.root / 'xrobot.lock')
        message = str(context.exception)
        self.assertIn('team/A is checked out at %s but xrobot.lock pins 000000000000' %
                      run_git(a, 'rev-parse', 'HEAD')[:12], message)
        self.assertIn('team/C is not a git checkout; run xrobot setup --frozen', message)
        self.assertIn('team/Gone from xrobot.lock is not checked out; run xrobot setup --frozen', message)
        self.assertNotIn('team/B', message)

    def test_a_lock_entry_without_a_commit_is_rejected(self):
        self.module('A', 'class A { public: A() {} };')
        self.write('xrobot.lock', 'version: 1\nmodules:\n  team/A: {repo: x}\n')
        with self.assertRaisesRegex(ValueError, 'xrobot.lock has no commit for team/A'):
            discover_modules(self.root / 'Modules', self.root / 'xrobot.lock')

    def test_a_lock_entry_leaving_the_modules_directory_is_rejected(self):
        self.write('xrobot.lock', 'version: 1\nmodules:\n  ../../outside: {commit: "%s"}\n' % ('0' * 40))
        with self.assertRaisesRegex(ValueError, 'Module path leaves directory'):
            discover_modules(self.root / 'Modules', self.root / 'xrobot.lock')

    def test_module_selection_by_short_name_or_id(self):
        self.module('A', 'class A { public: A() {} };')
        self.module('A', 'class A { public: A() {} };', owner='other')
        modules = discover_modules(self.root / 'Modules', self.root / 'xrobot.lock')
        self.assertEqual(select_module(modules, 'team/a')['id'], 'team/A')
        with self.assertRaisesRegex(ValueError, 'Ambiguous Module A; specify'):
            select_module(modules, 'A')
        with self.assertRaisesRegex(ValueError, 'Module not found: B'):
            select_module(modules, 'B')


class Manifests(unittest.TestCase):
    def test_allowed_keys(self):
        manifest = manifest_from_text(manifest_block('d', ['team/B@dev'], standalone=False), 'A.hpp')
        self.assertEqual((manifest.description, manifest.depends, manifest.standalone), ('d', ['team/B@dev'], False))
        self.assertEqual(manifest_from_text('/* === MODULE MANIFEST ===\ndescription: old\ndepends: team/B\n'
                                            '=== END MANIFEST === */').depends, ['team/B'])
        self.assertTrue(manifest_from_text('class A {};').standalone)

    def test_unknown_keys_are_rejected(self):
        for key in ('constructor_args', 'template_args', 'required_hardware'):
            with self.subTest(key=key), self.assertRaisesRegex(ValueError, r'unsupported manifest key\(s\) %s' % key):
                manifest_from_text(manifest_block('d', **{key: []}), 'A.hpp')

    def test_newer_manifest_versions_need_a_newer_tool(self):
        text = manifest_block('d', ['team/B@dev']).replace('MODULE MANIFEST V2', 'MODULE MANIFEST V3')
        self.assertIn('MODULE MANIFEST V3', text)
        with self.assertRaisesRegex(ValueError, r'A\.hpp: MODULE MANIFEST V3 needs a newer xrobot'):
            manifest_from_text(text, 'A.hpp')

    def test_multiple_manifests_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'multiple package manifests'):
            manifest_from_text(manifest_block() + manifest_block(), 'A.hpp')


class Catalogs(UpstreamTestCase):
    def test_validation_labels_are_bound_to_tested_versions(self):
        self.upstream('team/A')
        self.entries[0]['status'] = 'official'
        self.write_yaml(self.index, {'packages': self.entries})
        with self.assertRaisesRegex(ValueError, 'tested_ref and tested_libxr'):
            SourceManager(self.modules / 'sources.yaml')
        self.entries[0].update(tested_ref='2026-09-15', tested_libxr='6.0.0')
        self.write_yaml(self.index, {'packages': self.entries})
        self.assertEqual(SourceManager(self.modules / 'sources.yaml').packages['team/A']['tested_ref'], '2026-09-15')

    def test_unquoted_date_tags_are_kept_as_text(self):
        self.upstream('team/A')
        self.entries[0].update(status='verified', tested_ref='2026-09-15', tested_libxr='6.0.0')
        self.write_yaml(self.index, {'packages': self.entries})
        self.index.write_text(self.index.read_text(encoding='utf-8').replace("'2026-09-15'", '2026-09-15'),
                              encoding='utf-8')
        self.assertEqual(SourceManager(self.modules / 'sources.yaml').packages['team/A']['tested_ref'], '2026-09-15')

    def test_bsp_entries_are_discovery_metadata_only(self):
        self.write_yaml(self.index, {'bsps': ['https://github.com/team/board.git']})
        manager = SourceManager(self.modules / 'sources.yaml')
        record = manager.packages['team/board']
        self.assertEqual(set(record), {'id', 'type', 'repo', 'source', 'canonical'})
        self.assertEqual(manager.resolve_id('board', 'bsp'), 'team/board')
        self.assertEqual(manager.list_modules(), [])

    def test_equal_priority_sources_must_agree(self):
        self.upstream('team/A')
        other = self.write_yaml(self.tmp / 'other.yaml', {'packages': [
            {'id': 'team/A', 'type': 'module', 'repo': 'https://x.invalid/A'}]})
        self.write_yaml(self.modules / 'sources.yaml', {'sources': [{'url': str(self.index)}, {'url': str(other)}]})
        with self.assertRaisesRegex(ValueError, 'Equal-priority sources disagree about team/A'):
            SourceManager(self.modules / 'sources.yaml')

    def test_traversal_identities_are_rejected(self):
        for identity in ('../../out', 'team/../out', 'team/.', 'team/..', 'team/-bad', '/tmp/evil', 'A'):
            with self.subTest(identity=identity), self.assertRaises(ValueError):
                validate_id(identity)
        self.assertEqual(validate_id('team/A.b-c_1'), 'team/A.b-c_1')


if __name__ == '__main__':
    unittest.main()
