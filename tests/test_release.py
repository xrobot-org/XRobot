import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

_spec = importlib.util.spec_from_file_location('release_checker', Path(__file__).parents[1]/'tools/check_release.py')
release = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(release)


class ReleaseCorrespondence(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.repos = {}
        self.record = {'version': '6.0.0rc1', 'commits': {}, 'checks': dict.fromkeys(release.GATES, 'pass')}
        for name in ('libxr', 'xrobot', 'codegen'):
            path = self.root/name
            path.mkdir()
            self.repos[name] = path
            self.git(path, 'init', '-b', 'dev')
            self.git(path, 'config', 'user.name', 'Test')
            self.git(path, 'config', 'user.email', 'test@example.invalid')
            self.git(path, 'config', 'commit.gpgsign', 'false')
            self.git(path, 'config', 'core.autocrlf', 'false')
            (path/'pyproject.toml').write_text('[project]\nname = "%s"\nversion = "6.0.0rc1"\n' % name)
            self.git(path, 'add', '.')
            self.git(path, 'commit', '-m', 'fixture')
            self.record['commits'][name] = self.git(path, 'rev-parse', 'HEAD')

    def git(self, path, *args):
        p = subprocess.run(['git', '-C', str(path), *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=20)
        self.assertEqual(p.returncode, 0, p.stderr)
        return p.stdout.strip()

    def test_exact_candidates_pass_without_tagging(self):
        self.assertEqual(release.check('6.0.0rc1', self.repos, self.record), [])
        for path in self.repos.values():
            self.assertEqual(self.git(path, 'tag', '--list'), '')
            self.assertEqual(self.git(path, 'status', '--porcelain'), '')

    def test_missing_gate_fails(self):
        self.record['checks']['bsp'] = 'not-run'
        self.assertIn('Candidate acceptance is missing: bsp', release.check('6.0.0rc1', self.repos, self.record))

    def test_wrong_head_fails(self):
        self.record['commits']['libxr'] = '0'*40
        self.assertTrue(any('HEAD' in e for e in release.check('6.0.0rc1', self.repos, self.record)))

    def test_tag_alone_does_not_change_python_version(self):
        for path in self.repos.values():
            self.git(path, 'tag', 'v6.0.0')
        self.record['version'] = '6.0.0'
        errors = release.check('6.0.0', self.repos, self.record)
        self.assertEqual(sum('package version' in e for e in errors), 2)

    def test_dirty_candidate_fails(self):
        (self.repos['xrobot']/'uncommitted.py').write_text('# work\n')
        self.assertTrue(any('not a clean' in e for e in release.check('6.0.0rc1', self.repos, self.record)))


if __name__ == '__main__':
    unittest.main()
