"""Shared test fixtures.

- ``BspTestCase``: a temporary BSP (``Modules/modules.yaml``, ``User/``) whose
  Modules are real git checkouts committed in place and pinned by an
  ``xrobot.lock`` that points at their commits, as ``xrobot setup`` leaves them.
- ``UpstreamTestCase``: upstream Module repositories (branches ``master`` and
  ``dev``) listed in a local catalog index, plus an empty BSP whose
  ``Modules/sources.yaml`` points at that catalog, for resolution tests.
- ``CxxMixin``: compiles the generated header with ``$CXX`` (default ``g++``)
  against stub ``libxr.hpp``/``thread.hpp``; tests are skipped without it.
"""
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import yaml

from xrobot import __version__
from xrobot.Project import Project

GIT_OPTIONS = ['-c', 'commit.gpgsign=false', '-c', 'tag.gpgsign=false', '-c', 'core.autocrlf=false',
               '-c', 'init.defaultBranch=master', '-c', 'advice.detachedHead=false']
GIT_ENV = {
    'GIT_AUTHOR_NAME': 'Fixture', 'GIT_AUTHOR_EMAIL': 'fixture@example.invalid',
    'GIT_COMMITTER_NAME': 'Fixture', 'GIT_COMMITTER_EMAIL': 'fixture@example.invalid',
    'GIT_TERMINAL_PROMPT': '0', 'GIT_CONFIG_NOSYSTEM': '1',
}

CXX = os.environ.get('CXX', 'g++')
HAVE_CXX = shutil.which(CXX) is not None

LIBXR_STUB = '#pragma once\n'
THREAD_STUB = ('#pragma once\n#include <cstdlib>\n'
               'namespace LibXR { struct Thread { static void Sleep(unsigned) { std::_Exit(0); } }; }\n')


def run_git(repo, *args, check=True):
    """Run git with a fixed identity and no user/system configuration influence."""
    command = ['git'] + GIT_OPTIONS + (['-C', str(repo)] if repo is not None else []) + list(args)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding='utf-8',
                            errors='replace', env=dict(os.environ, **GIT_ENV), timeout=60)
    if check and result.returncode:
        raise AssertionError('git %s failed in %s:\n%s' % (' '.join(args), repo, result.stderr))
    return result.stdout.strip()


def manifest_block(description='fixture', depends=None, **extra):
    data = {'module_description': description}
    if depends is not None:
        data['depends'] = depends
    data.update(extra)
    return ('/* === MODULE MANIFEST V2 ===\n' + yaml.safe_dump(data, sort_keys=False) +
            '=== END MANIFEST === */\n')


class TempDirTestCase(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self._temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self._temporary.cleanup)
        self.tmp = Path(self._temporary.name).resolve()

    def write(self, path, text):
        path = Path(path)
        if not path.is_absolute():
            path = self.root / path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode('utf-8'))
        return path

    def read(self, path):
        path = Path(path)
        if not path.is_absolute():
            path = self.root / path
        return path.read_bytes().decode('utf-8')


class BspTestCase(TempDirTestCase):
    """A BSP whose Modules are git checkouts pinned by xrobot.lock."""

    owner = 'team'

    def setUp(self):
        super().setUp()
        self.root = self.tmp / 'bsp'
        self.locked = {}
        self.write('Modules/modules.yaml', 'xrobot: %s\nmodules: []\n' % __version__)
        (self.root / 'User').mkdir(parents=True)
        self.write_lock()

    @property
    def project(self):
        return Project(self.root)

    def module(self, name, body, owner=None, manifest=None, extra_headers=None):
        """Write Modules/<owner>/<name>/<name>.hpp, commit it and pin it in the lock."""
        owner = owner or self.owner
        identity = '%s/%s' % (owner, name)
        folder = self.root / 'Modules' / owner / name
        text = '#pragma once\n' + (manifest if manifest is not None else '') + body + '\n'
        self.write(folder / (name + '.hpp'), text)
        for header, content in (extra_headers or {}).items():
            self.write(folder / header, content)
        if not (folder / '.git').exists():
            run_git(folder, 'init', '-q')
        run_git(folder, 'add', '-A')
        run_git(folder, 'commit', '-q', '--allow-empty', '-m', 'fixture')
        self.locked[identity] = run_git(folder, 'rev-parse', 'HEAD')
        self.write_lock()
        return folder

    def write_lock(self):
        data = {'version': 1, 'requests': [{'id': i, 'ref': None} for i in sorted(self.locked)],
                'modules': {i: {'repo': 'https://example.invalid/%s.git' % i, 'commit': c}
                            for i, c in sorted(self.locked.items())}}
        self.write('xrobot.lock', yaml.safe_dump(data, sort_keys=False))

    def entry(self, text, name='app_main.cpp'):
        return self.write('User/' + name, text)

    def config(self, data, name='xrobot.yaml'):
        text = data if isinstance(data, str) else yaml.safe_dump(data, sort_keys=False)
        return self.write('User/' + name, text)

    def generate(self, data=None, entry=None, name='xrobot.yaml'):
        """Write the config (and entry) and generate User/xrobot_main.hpp; return its text."""
        from xrobot.GenerateMain import generate
        if entry is not None or not (self.root / 'User/app_main.cpp').exists():
            self.entry(entry if entry is not None else
                       '#include "xrobot_main.hpp"\nint main() { XROBOT_MAIN(); }\n')
        path = self.config(data, name) if data is not None else self.root / 'User' / name
        return generate(self.project, path)


class CxxMixin:
    """Compile (and run) the entry against the generated header."""

    standard = os.environ.get('XR_CXX_STANDARD', 'c++20')

    def setUp(self):
        super().setUp()
        self.stub('libxr.hpp', LIBXR_STUB)
        self.stub('thread.hpp', THREAD_STUB)

    def stub(self, name, text):
        self.write(self.tmp / 'stub' / name, text)

    def include_flags(self):
        flags = ['-I' + str(self.tmp / 'stub'), '-I' + str(self.root / 'User')]
        modules = self.root / 'Modules'
        for owner in sorted(p for p in modules.iterdir() if p.is_dir()):
            flags += ['-I' + str(p) for p in sorted(owner.iterdir()) if p.is_dir()]
        return flags

    def compile(self, source=None, expected=True, execute=True, extra=(), warnings=True):
        source = Path(source) if source else self.root / 'User/app_main.cpp'
        output = self.tmp / ('program.exe' if execute else 'object.o')
        command = [CXX, '-std=' + self.standard]
        if warnings:
            command += ['-Wall', '-Wextra', '-Werror']
        command += ['-O1'] + self.include_flags() + list(extra)
        if not execute:
            command.append('-c')
        command += [str(source), '-o', str(output)]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                encoding='utf-8', errors='replace', timeout=120)
        if not expected:
            self.assertNotEqual(result.returncode, 0, 'compilation unexpectedly succeeded')
            return result.stdout
        self.assertEqual(result.returncode, 0, result.stdout)
        if execute:
            run = subprocess.run([str(output)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 encoding='utf-8', errors='replace', timeout=30)
            self.assertEqual(run.returncode, 0, run.stdout)
            return run.stdout
        return result.stdout


requires_cxx = unittest.skipUnless(HAVE_CXX, 'C++ compiler %s not available on this host' % CXX)


class UpstreamTestCase(TempDirTestCase):
    """Upstream Module repositories in a local catalog, and an empty BSP using it."""

    def setUp(self):
        super().setUp()
        self.root = self.tmp / 'bsp'
        self.modules = self.root / 'Modules'
        self.modules.mkdir(parents=True)
        self.index = self.tmp / 'index.yaml'
        self.entries = []
        self.write_yaml(self.modules / 'sources.yaml', {'sources': [{'url': str(self.index)}]})
        self.write_yaml(self.index, {'packages': []})
        self.configure([])

    @property
    def project(self):
        return Project(self.root)

    def write_yaml(self, path, value):
        return self.write(path, yaml.safe_dump(value, sort_keys=False))

    def upstream(self, identity, depends=None, kind='module', branches=('dev',), catalog=True):
        """Create an upstream repository on master (plus ``branches``) and list it in the catalog."""
        path = self.tmp / 'upstream' / identity
        path.mkdir(parents=True)
        run_git(path, 'init', '-q', '-b', 'master')
        self.commit(path, depends or [], 'initial')
        for branch in branches:
            run_git(path, 'branch', branch)
        if catalog:
            self.entries.append({'id': identity, 'repo': str(path), 'type': kind})
            self.write_yaml(self.index, {'packages': self.entries})
        return path

    def commit(self, path, depends, message):
        name = Path(path).name
        text = ('#pragma once\n' + manifest_block(message, depends) +
                'class %s { public: %s() {} };\n' % (name, name))
        self.write(Path(path) / (name + '.hpp'), text)
        self.write(Path(path) / 'CMakeLists.txt', 'target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")\n')
        run_git(path, 'add', '-A')
        run_git(path, 'commit', '-q', '-m', message)
        return run_git(path, 'rev-parse', 'HEAD')

    def configure(self, requests, pin=__version__):
        data = {}
        if pin is not None:
            data['xrobot'] = pin
        data['modules'] = requests
        self.write_yaml(self.modules / 'modules.yaml', data)

    def sync(self, cwd=None, **kwargs):
        """Run sync_modules from ``cwd`` (default: the BSP root, where `xrobot setup` usually runs)."""
        from xrobot.InitModule import sync_modules
        previous = os.getcwd()
        os.chdir(str(cwd or self.root))
        try:
            return sync_modules(self.project, **kwargs)
        finally:
            os.chdir(previous)

    def lock_bytes(self):
        return (self.root / 'xrobot.lock').read_bytes()

    def head(self, identity):
        return run_git(self.modules / identity, 'rev-parse', 'HEAD')
