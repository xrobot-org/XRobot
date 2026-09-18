import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import yaml

from xrobot.SourceManager import SourceManager, ModuleSource, validate_id
from xrobot.InitModule import sync_modules_by_config
from xrobot.XRobotSetup import setup


class Packages(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.project = self.root / 'project'
        self.modules = self.project / 'Modules'
        self.modules.mkdir(parents=True)
        self.config = self.modules / 'modules.yaml'
        self.sources = self.modules / 'sources.yaml'
        self.index = self.root / 'index.yaml'
        self.entries = []
        self.write(self.sources, {'sources': [{'url': str(self.index)}]})
        self.git_env = dict(os.environ, GIT_CONFIG_NOSYSTEM='1', GIT_TERMINAL_PROMPT='0')

    def write(self, path, value):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(yaml.safe_dump(value, sort_keys=False), encoding='utf-8')

    def git(self, repo, *args):
        result = subprocess.run(['git', '-C', str(repo), *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, encoding='utf-8', env=self.git_env, timeout=20)
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout.strip()

    def source(self, identity, deps=None, kind='module'):
        path = self.root / 'upstream' / identity
        path.mkdir(parents=True)
        self.git(path, 'init', '-b', 'master')
        self.git(path, 'config', 'user.name', 'Fixture')
        self.git(path, 'config', 'user.email', 'fixture@example.invalid')
        self.git(path, 'config', 'commit.gpgsign', 'false')
        self.git(path, 'config', 'core.autocrlf', 'false')
        self.commit(path, deps or [], 'initial')
        self.git(path, 'branch', 'dev')
        self.entries.append({'id': identity, 'repo': str(path), 'type': kind})
        self.write(self.index, {'namespace': 'fixtures', 'packages': self.entries})
        return path

    def commit(self, path, deps, message):
        name = path.name
        manifest = yaml.safe_dump({'module_description': message, 'depends': deps}, sort_keys=False)
        (path / (name + '.hpp')).write_text('#pragma once\n/* === MODULE MANIFEST V2 ===\n' + manifest + '=== END MANIFEST === */\nclass %s { public: %s() {} };\n' % (name, name), encoding='utf-8')
        (path / 'CMakeLists.txt').write_text('target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")\n')
        self.git(path, 'add', '.')
        self.git(path, 'commit', '-m', message)
        return self.git(path, 'rev-parse', 'HEAD')

    def configure(self, roots):
        self.write(self.config, {'modules': roots})

    def resolve(self, **kwargs):
        return sync_modules_by_config(self.config, self.sources, self.modules, **kwargs)

    def test_nested_sources_and_exact_lock(self):
        b = self.source('team/B')
        a = self.source('team/A', ['team/B@master'])
        self.configure(['team/A@master'])
        lock = self.resolve()
        self.assertEqual(set(lock['modules']), {'team/A', 'team/B'})
        self.assertEqual(lock['modules']['team/B']['commit'], self.git(b, 'rev-parse', 'HEAD'))
        self.assertTrue((self.modules / 'team/A/A.hpp').exists())
        self.assertIn('team/B/CMakeLists.txt', (self.modules / 'CMakeLists.txt').read_text())
        self.assertFalse((self.modules / 'team/A/xrobot.lock').exists())

    def test_lock_is_stable_update_is_explicit(self):
        a = self.source('team/A')
        self.configure(['team/A@master'])
        old = self.resolve()['modules']['team/A']['commit']
        new = self.commit(a, [], 'change')
        self.assertNotEqual(old, new)
        self.assertEqual(self.resolve()['modules']['team/A']['commit'], old)
        self.assertEqual(self.resolve(frozen=True)['modules']['team/A']['commit'], old)
        self.assertEqual(self.resolve(update=True)['modules']['team/A']['commit'], new)

    def test_offline_needs_neither_index_nor_remote(self):
        a = self.source('team/A')
        self.configure(['team/A'])
        old = self.resolve()['modules']['team/A']['commit']
        self.index.unlink()
        self.sources.unlink()
        a.rename(a.with_name('unavailable'))
        self.assertEqual(self.resolve(offline=True)['modules']['team/A']['commit'], old)

    def test_missing_or_changed_lock_fails(self):
        self.source('team/A')
        self.configure(['team/A'])
        with self.assertRaisesRegex(ValueError, 'No project lock'):
            self.resolve(frozen=True)
        self.resolve()
        self.configure(['team/A@dev'])
        with self.assertRaisesRegex(ValueError, 'requests differ'):
            self.resolve()

    def test_stacked_branch_then_fallback_without_manifest_edit(self):
        b = self.source('team/B')
        a = self.source('team/A', [{'id': 'team/B', 'ref': 'same-or-dev'}])
        self.git(b, 'checkout', '-b', 'feature/next')
        feature = self.commit(b, [], 'feature')
        self.git(a, 'checkout', '-b', 'feature/next')
        self.configure(['team/A@feature/next'])
        first = self.resolve()
        self.assertEqual(first['modules']['team/B']['commit'], feature)
        self.assertEqual(first['modules']['team/B']['resolved_ref'], 'feature/next')
        self.git(b, 'checkout', 'dev')
        self.git(b, 'merge', '--ff-only', 'feature/next')
        self.git(b, 'branch', '-d', 'feature/next')
        second = self.resolve(update=True)
        self.assertEqual(second['modules']['team/B']['commit'], feature)
        self.assertEqual(second['modules']['team/B']['resolved_ref'], 'dev')
        self.assertEqual(first['modules']['team/A']['commit'], second['modules']['team/A']['commit'])

    def test_tag_never_falls_back_to_dev(self):
        b = self.source('team/B')
        a = self.source('team/A', [{'id':'team/B','ref':'same-or-dev'}])
        self.git(a, 'tag', '2026-09-15')
        self.configure(['team/A@2026-09-15'])
        with self.assertRaisesRegex(ValueError, 'same tag is missing'):
            self.resolve()
        self.assertFalse((self.project/'xrobot.lock').exists())
        self.git(b, 'tag', '2026-09-15')
        lock = self.resolve()
        self.assertEqual(lock['modules']['team/B']['ref_kind'], 'tag')
        self.assertEqual(lock['modules']['team/B']['resolved_ref'], '2026-09-15')

    def test_detached_pr_head_requires_logical_context(self):
        b = self.source('team/B')
        a = self.source('team/A', [{'id':'team/B','ref':'same-or-dev'}])
        sha = self.git(a,'rev-parse','HEAD')
        self.configure(['team/A@'+sha])
        with self.assertRaisesRegex(ValueError, 'explicit parent'):
            self.resolve()
        self.configure([{'id':'team/A','ref':sha,'context_ref':'refs/heads/pr-feature'}])
        self.assertEqual(self.resolve()['modules']['team/B']['resolved_ref'],'dev')

    def test_relative_roots_use_the_bsp_branch_and_preserve_it_in_lock(self):
        a = self.source('team/A')
        self.git(a, 'branch', 'feature/board')
        self.git(self.project, 'init', '-b', 'feature/board')
        self.configure(['team/A@same-or-dev'])
        first = self.resolve()
        self.assertEqual(first['modules']['team/A']['resolved_ref'], 'feature/board')
        self.assertEqual(first['root_context'], ['branch', 'feature/board'])
        self.assertEqual(self.resolve(offline=True), first)
        self.git(self.project, 'symbolic-ref', 'HEAD', 'refs/heads/other')
        with self.assertRaisesRegex(ValueError, 'root context differs'):
            self.resolve(frozen=True)
        self.assertEqual(self.resolve(update=True)['modules']['team/A']['resolved_ref'], 'dev')

    def test_relative_root_tag_is_exact_and_never_falls_back(self):
        a = self.source('team/A')
        self.configure(['team/A@same-or-dev'])
        with self.assertRaisesRegex(ValueError, 'same tag is missing'):
            self.resolve(context_ref='refs/tags/release-check')
        self.git(a, 'tag', 'release-check')
        first = self.resolve(context_ref='refs/tags/release-check')
        self.assertEqual(first['modules']['team/A']['ref_kind'], 'tag')
        self.assertEqual(self.resolve(context_ref='refs/tags/release-check', offline=True), first)

    def test_relative_root_without_git_requires_explicit_context(self):
        self.source('team/A')
        self.configure(['team/A@same-or-dev'])
        with self.assertRaisesRegex(ValueError, 'Relative root dependencies require'):
            self.resolve()
        first = self.resolve(context_ref='refs/heads/review')
        self.assertEqual(first['modules']['team/A']['resolved_ref'], 'dev')
        self.assertEqual(first['root_context'], ['branch', 'review'])

    def test_per_root_context_is_supported_without_a_bsp_identity_file(self):
        self.source('team/A')
        self.configure([{'id': 'team/A', 'ref': 'same', 'context_ref': 'refs/heads/dev'}])
        first = self.resolve()
        self.assertEqual(first['modules']['team/A']['resolved_ref'], 'dev')
        self.assertNotIn('root_context', first)
        self.assertEqual(self.resolve(offline=True), first)

    def test_generated_ci_preparation_keeps_pr_head_and_local_index_priority(self):
        from unittest.mock import patch
        from xrobot.ModuleWorkflow import _PREPARE_SOURCES
        b = self.source('team/B')
        a = self.source('team/A', ['team/B@same-or-dev'])
        selected = self.git(a, 'rev-parse', 'HEAD')
        local = self.modules/'team/A'
        local.parent.mkdir(parents=True)
        self.git(self.project, 'clone', str(a), str(local))
        self.git(local, 'checkout', '--detach', selected)
        self.commit(a, ['team/B@same-or-dev'], 'new remote head')
        class CatalogResponse:
            def __init__(self, data):
                self.text = yaml.safe_dump(data)
            def raise_for_status(self):
                pass
        def catalog(url, **kwargs):
            if url == 'https://xrobot.work/xrobot-modules/index.yaml':
                return CatalogResponse({'packages': [
                    {'id': 'team/A', 'type': 'module', 'repo': (self.root/'wrong-upstream').as_uri()},
                    {'id': 'team/B', 'type': 'module', 'repo': b.as_uri()},
                ]})
            if url == 'https://qdu-robomaster.github.io/qdu-future-modules/index.yaml':
                return CatalogResponse({'modules': []})
            raise AssertionError('Unexpected catalog request: ' + url)
        script = '\n'.join(_PREPARE_SOURCES.splitlines()[1:-1])
        previous = Path.cwd()
        try:
            os.chdir(self.project)
            with patch.dict(os.environ, XR_MODULE_ID='team/A', XR_REF_KIND='branch',
                            XR_REF_NAME='feature/ci', XR_TEMPLATE_ARGS='[]'):
                with patch('xrobot.SourceManager.requests.get', side_effect=catalog):
                    exec(compile(script, '<generated CI source preparation>', 'exec'), {'__name__': '__ci_test__'})
        finally:
            os.chdir(previous)
        lock = yaml.safe_load((self.project/'xrobot.lock').read_text(encoding='utf-8'))
        self.assertEqual(lock['modules']['team/A']['commit'], selected)
        self.assertEqual(lock['modules']['team/A']['context'], ['branch', 'feature/ci'])
        self.assertEqual(lock['modules']['team/B']['resolved_ref'], 'dev')
        self.assertEqual(self.git(local, 'rev-parse', 'HEAD'), selected)
        probe = (self.project/'module_check.cpp').read_text()
        self.assertIn('void XRobotCompileCheck()', probe)
        self.assertIn('static A module_0;', probe)

    def test_short_names_are_unambiguous_only(self):
        self.source('first/A')
        self.source('second/A')
        self.configure(['A'])
        with self.assertRaisesRegex(ValueError,'Ambiguous.*first/A.*second/A'):
            self.resolve()
        self.configure(['first/A'])
        self.assertEqual(set(self.resolve()['modules']),{'first/A'})

    def test_bsp_discovery_without_dependency_checkout(self):
        self.source('team/Board',kind='bsp')
        manager=SourceManager(self.sources)
        self.assertEqual(manager.resolve_id('Board','bsp'),'team/Board')
        self.assertEqual(manager.list_modules(),[])
        self.configure(['team/Board'])
        with self.assertRaisesRegex(ValueError,'BSP catalog'):
            self.resolve()
        self.assertFalse((self.modules/'team/Board').exists())

    def test_conflict_leaves_lock_and_checkout_unchanged(self):
        b=self.source('team/B')
        self.git(b,'tag','old')
        old=self.git(b,'rev-parse','HEAD')
        self.commit(b,[],'new')
        self.git(b,'tag','new')
        self.source('team/A',['team/B@old'])
        self.source('team/C',['team/B@new'])
        self.configure(['team/A'])
        self.resolve()
        old_lock=(self.project/'xrobot.lock').read_bytes()
        self.configure(['team/A','team/C'])
        with self.assertRaisesRegex(ValueError,'Dependency conflict'):
            self.resolve(update=True)
        self.assertEqual((self.project/'xrobot.lock').read_bytes(),old_lock)
        self.assertEqual(self.git(self.modules/'team/B','rev-parse','HEAD'),old)

    def test_cycle_is_reported(self):
        self.source('team/A',['team/B'])
        self.source('team/B',['team/A'])
        self.configure(['team/A'])
        with self.assertRaisesRegex(ValueError,'cycle'):
            self.resolve()

    def test_local_changes_are_never_overwritten(self):
        a=self.source('team/A')
        self.configure(['team/A'])
        self.resolve()
        path=self.modules/'team/A/A.hpp'
        path.write_text(path.read_text()+'\n// local work\n')
        before=path.read_bytes()
        self.commit(a,[],'next')
        with self.assertRaisesRegex(ValueError,'Local changes preserved'):
            self.resolve(update=True)
        self.assertEqual(path.read_bytes(),before)

    def test_branch_tag_ambiguity(self):
        a=self.source('team/A')
        self.git(a,'tag','dev')
        self.configure(['team/A@dev'])
        with self.assertRaisesRegex(ValueError,'branch and tag'):
            self.resolve()
        self.configure(['team/A@refs/heads/dev'])
        self.assertEqual(self.resolve()['modules']['team/A']['ref_kind'],'branch')

    def test_relative_index_and_local_source(self):
        a=self.source('team/A')
        self.entries[0]['repo']='upstream/team/A'
        self.write(self.index,{'packages':self.entries})
        self.write(self.sources,{'sources':[{'url':'../../index.yaml'}]})
        self.configure(['team/A'])
        first = self.resolve()
        row = first['modules']['team/A']
        self.assertEqual(row['repo'], '../upstream/team/A')
        self.assertEqual(row['source'], '../index.yaml')
        self.assertNotIn(str(self.root), (self.project/'xrobot.lock').read_text())
        self.assertEqual(self.resolve(offline=True), first)

    def test_local_file_uri_lock_reopens_without_machine_paths(self):
        a = self.source('team/A')
        self.entries[0]['repo'] = a.as_uri()
        self.write(self.index, {'packages': self.entries})
        self.configure(['team/A@dev'])
        first = self.resolve()
        self.assertEqual(first['modules']['team/A']['repo'], '../upstream/team/A')
        self.assertEqual(self.resolve(offline=True), first)
        self.assertNotIn('file://', (self.project/'xrobot.lock').read_text())

    def test_catalog_verification_is_version_bound(self):
        self.source('team/A')
        self.entries[0]['status']='official'
        self.write(self.index,{'packages':self.entries})
        with self.assertRaisesRegex(ValueError,'tested_ref and tested_libxr'):
            SourceManager(self.sources)
        self.entries[0].update(tested_ref='2026-09-15',tested_libxr='6.2.1')
        self.write(self.index,{'packages':self.entries})
        self.assertEqual(SourceManager(self.sources).packages['team/A']['tested_ref'],'2026-09-15')

    def test_catalog_date_tag_is_normalized(self):
        self.source('team/A')
        self.entries[0].update(status='verified', tested_ref='2026-09-15', tested_libxr='6.0.0')
        self.write(self.index, {'packages': self.entries})
        self.index.write_text(self.index.read_text().replace("'2026-09-15'", '2026-09-15'))
        manager = SourceManager(self.sources)
        self.assertEqual(manager.packages['team/A']['tested_ref'], '2026-09-15')
        import json
        json.dumps(manager.packages)

    def test_traversal_identity_rejected(self):
        for identity in ['../../out','team/../out','team/.','team/..','team/-bad','/tmp/evil']:
            with self.subTest(identity=identity), self.assertRaises(ValueError):
                validate_id(identity)

    def test_incomplete_lock_fails(self):
        self.source('team/B')
        self.source('team/A',['team/B'])
        self.configure(['team/A'])
        lock=self.resolve()
        del lock['modules']['team/B']
        self.write(self.project/'xrobot.lock',lock)
        with self.assertRaisesRegex(ValueError,'lock.*missing|missing.*lock'):
            self.resolve(offline=True)


    def test_setup_uses_selected_instance_config(self):
        self.source('team/A')
        self.configure(['team/A'])
        user=self.project/'User'
        user.mkdir()
        self.write(user/'xrobot.yaml',{'modules':[]})
        self.write(user/'alternative.yaml',{'modules':[{'module':'team/A','id':'a0'}]})
        self.assertTrue(setup(self.project,user/'alternative.yaml'))
        self.assertIn('A a0;', (user/'xrobot_main.hpp').read_text())
        self.assertFalse((self.project/'build').exists())

    def test_lock_directory_is_derived_not_persisted(self):
        self.source('team/A')
        self.configure(['team/A'])
        lock = self.resolve()
        self.assertNotIn('directory', lock['modules']['team/A'])
        self.assertEqual(self.resolve(frozen=True), lock)
        self.assertIn('team/A', (self.project/'Modules/CMakeLists.txt').read_text())
        lock['modules']['team/A']['directory'] = 'team/A'
        self.write(self.project/'xrobot.lock', lock)
        with self.assertRaisesRegex(ValueError, 'Legacy lock directory'):
            self.resolve(frozen=True)

    def test_bsp_catalog_accepts_only_repository_url(self):
        self.write(self.index, {'bsps': ['https://github.com/team/board.git']})
        manager = SourceManager(self.sources)
        record = manager.packages['team/board']
        self.assertEqual(record['type'], 'bsp')
        self.assertEqual(set(record), {'id', 'type', 'repo', 'source'})
        self.assertEqual(manager.resolve_id('board', 'bsp'), 'team/board')


if __name__ == '__main__':
    unittest.main()
