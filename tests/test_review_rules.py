"""Generation rules added by the 2026-09-24 toolchain review.

Each test pins one rule: complete mappings (R3), constructor-named mappings for
non-aggregates (R6), no conditional registrations (R8), lock-authoritative module
discovery (R12), restored constexprs and a self-contained header (R13/R14), the
input stamp (R5) and comment-preserving edits (R10).
"""
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import yaml

from xrobot.AddModule import append_module_instance
from xrobot.GenerateMain import generate, generate_xrobot_main_code, load_config, read_registrations
from xrobot.ModuleParser import discover_modules


class Fixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / 'Modules').mkdir()
        (self.root / 'User').mkdir()

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding='utf-8')
        return path

    def module(self, name, text):
        self.write('Modules/%s/%s.hpp' % (name, name), '#pragma once\n' + text)

    def code(self, config, main='int main(){}\n'):
        self.write('User/app_main.cpp', main)
        path = self.write('User/xrobot.yaml', yaml.safe_dump(config, sort_keys=False))
        return generate_xrobot_main_code(load_config(path), discover_modules(self.root / 'Modules'),
                                         read_registrations([self.root / 'User/app_main.cpp']))


class CompleteMappings(Fixture):
    def test_designated_default_requires_every_field_in_order(self):
        self.module('P', 'class P { public: struct Param { int a; int b; };\n'
                         '  explicit P(const Param& param = {.a = 1, .b = 2}) {} };')
        base = {'module': 'P', 'id': 'p'}
        self.assertIn('.b = 3', self.code({'modules': [dict(base, args=[{'param': {'a': 1, 'b': 3}}])]}))
        with self.assertRaisesRegex(ValueError, r'p\.args\.param: missing b'):
            self.code({'modules': [dict(base, args=[{'param': {'a': 1}}])]})
        with self.assertRaisesRegex(ValueError, 'out of declaration order'):
            self.code({'modules': [dict(base, args=[{'param': {'b': 3, 'a': 1}}])]})

    def test_factory_default_uses_the_struct_definition(self):
        self.module('Q', 'class Q { public:\n'
                         '  struct Config { int kept{1}; const char* topic{"x"}; };\n'
                         '  static Config DefaultConfig() { return {}; }\n'
                         '  explicit Q(Config cfg = DefaultConfig()) {} };')
        base = {'module': 'Q', 'id': 'q'}
        with self.assertRaisesRegex(ValueError, r'q\.args\.cfg: missing topic \(data members of Q::Config\)'):
            self.code({'modules': [dict(base, args=[{'cfg': {'kept': 2}}])]})
        self.assertIn('.topic = "y"', self.code({'modules': [dict(base, args=[{'cfg': {'kept': 2, 'topic': '"y"'}}])]}))

    def test_nested_and_cross_header_structs_are_checked(self):
        self.module('Shared', 'namespace Shared { struct Preview { bool enabled{false}; int port{1}; }; }\n'
                              'class Shared_ { public: Shared_() {} };')
        self.module('R', '#include "Shared.hpp"\nclass R { public:\n'
                         '  struct Config { struct Inner { int x{0}; } inner; Shared::Preview preview{}; };\n'
                         '  explicit R(Config cfg = {}) {} };')
        base = {'module': 'R', 'id': 'r'}
        with self.assertRaisesRegex(ValueError, r'r\.args\.cfg\.preview: missing port'):
            self.code({'modules': [dict(base, args=[{'cfg': {'inner': {'x': 1}, 'preview': {'enabled': True}}}])]})
        with self.assertRaisesRegex(ValueError, r'r\.args\.cfg\.inner: missing x'):
            self.code({'modules': [dict(base, args=[{'cfg': {'inner': {}, 'preview': {'enabled': True, 'port': 2}}}])]})

    def test_unlocatable_type_rejects_a_mapping_but_accepts_cpp_text(self):
        self.module('U', '#include <external.hpp>\nclass U { public: explicit U(External::Cfg cfg = {}) {} };')
        with self.assertRaisesRegex(ValueError, 'write this value as a complete C\\+\\+ expression'):
            self.code({'modules': [{'module': 'U', 'id': 'u', 'args': [{'cfg': {'a': 1}}]}]})
        self.assertIn('External::Cfg{1}', self.code(
            {'modules': [{'module': 'U', 'id': 'u', 'args': [{'cfg': 'External::Cfg{1}'}]}]}))


class ConstructorMappings(Fixture):
    def setUp(self):
        super().setUp()
        self.module('S', 'class S { public:\n'
                         '  enum class Mode { A, B };\n'
                         '  struct Runtime {\n'
                         '    Runtime() = default;\n'
                         '    Runtime(Mode mode, unsigned level, unsigned long long settle = 10U) {}\n'
                         '    Runtime(Mode mode, unsigned legacy_div, unsigned level, float legacy_hz = 1.0F) {}\n'
                         '  };\n'
                         '  explicit S(Runtime runtime = {}) {} };')

    def test_names_select_one_constructor_with_exact_types(self):
        code = self.code({'modules': [{'module': 'S', 'id': 's', 'args': [{'runtime': {
            'mode': 'S::Mode::A', 'legacy_div': 3, 'level': 1, 'legacy_hz': '100.0F'}}]}]})
        self.assertIn('S::Runtime(', code)
        self.assertIn('static_cast<S::Mode>(S::Mode::A)', code)
        self.assertIn('static_cast<float>(100.0F)', code)

    def test_unknown_names_list_the_constructors(self):
        with self.assertRaisesRegex(ValueError, 'mode, level, settle \\| mode, legacy_div, level, legacy_hz'):
            self.code({'modules': [{'module': 'S', 'id': 's', 'args': [{'runtime': {'mode': 'S::Mode::A', 'level': 1}}]}]})


class Registrations(Fixture):
    def test_conditional_registration_is_rejected(self):
        self.module('M', 'class M { public: M() {} };')
        main = 'int x;\n#if defined(OPTION)\nXR_REGISTER(x, int);\n#endif\n'
        with self.assertRaisesRegex(ValueError, 'XR_REGISTER inside #if'):
            self.code({'modules': [{'module': 'M', 'id': 'm'}]}, main)


class ConstantsAndHeader(Fixture):
    def test_constexprs_are_emitted_and_header_includes_libxr(self):
        self.module('C', 'class C { public: explicit C(int n = 1) {} };')
        code = self.code({'constexpr_namespace': 'Board', 'constexpr_includes': ['board.hpp'],
                          'constexprs': {'Rate': {'type': 'int', 'value': '250'}},
                          'modules': [{'module': 'C', 'id': 'c', 'args': [{'n': 'Board::Rate'}]}]})
        self.assertIn('#include "libxr.hpp"', code)
        self.assertIn('#include "board.hpp"', code)
        self.assertIn('namespace Board {\ninline constexpr int Rate = 250;\n}', code)
        self.assertLess(code.index('inline constexpr int Rate'), code.index('void XRobotMain('))


class Stamp(Fixture):
    def test_stamp_records_normalized_input_hashes(self):
        self.module('C', 'class C { public: C() {} };')
        self.write('User/app_main.cpp', 'int main(){}\n')
        config = self.root / 'User/xrobot.yaml'
        config.write_bytes(b'modules:\r\n- module: C\r\n  id: c\r\n')
        lock = self.root / 'xrobot.lock'
        lock.write_bytes(b'version: 1\r\nmodules:\r\n  C: {}\r\n')
        code = generate(config, self.root / 'Modules', self.root / 'User/xrobot_main.hpp', [], lock)
        digest = hashlib.sha256(config.read_bytes().replace(b'\r\n', b'\n')).hexdigest()
        self.assertIn('// xrobot-stamp: config=xrobot.yaml sha256=' + digest, code)
        self.assertIn('// xrobot-stamp: lock=../xrobot.lock sha256=' +
                      hashlib.sha256(b'version: 1\nmodules:\n  C: {}\n').hexdigest(), code)
        self.assertIn('// xrobot-stamp: tool=xrobot ', code)


    def test_inputs_on_another_drive_are_stamped_by_absolute_path(self):
        from unittest import mock
        from xrobot.GenerateMain import stamp_lines, stamp_state
        config = self.write('User/xrobot.yaml', 'modules: []\n')
        header = self.root / 'User/xrobot_main.hpp'
        with mock.patch('xrobot.GenerateMain.os.path.relpath', side_effect=ValueError('path is on mount C:')):
            stamp = stamp_lines(header, config, self.root / 'xrobot.lock')
        self.assertIn('config=%s sha256=' % Path(os.path.abspath(config)).as_posix(), stamp)
        header.write_text('#pragma once\n' + stamp, encoding='utf-8')
        self.assertEqual(stamp_state(header)['status'], 'fresh')


class LockedDiscovery(Fixture):
    def git(self, *args):
        subprocess.run(['git', '-C', str(self.root / 'Modules/team/A'), *args], check=True,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       env=dict(os.environ, GIT_CONFIG_NOSYSTEM='1'))

    def test_only_locked_modules_at_locked_commits_are_used(self):
        self.write('Modules/team/A/A.hpp', 'class A { public: A() {} };\n')
        self.write('Modules/stale/B/B.hpp', 'class B { public: B() {} };\n')
        self.git('init', '-b', 'master')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', 'add', '.')
        self.git('-c', 'user.name=t', '-c', 'user.email=t@t', '-c', 'commit.gpgsign=false', 'commit', '-m', 'a')
        head = subprocess.check_output(['git', '-C', str(self.root / 'Modules/team/A'), 'rev-parse', 'HEAD'], text=True).strip()
        self.write('xrobot.lock', yaml.safe_dump({'version': 1, 'modules': {'team/A': {'commit': head}}}))
        self.assertEqual(set(discover_modules(self.root / 'Modules')), {'team/A'})
        self.write('xrobot.lock', yaml.safe_dump({'version': 1, 'modules': {'team/A': {'commit': '0' * 40}}}))
        with self.assertRaisesRegex(ValueError, 'xrobot_setup --frozen.*xrobot_setup --update'):
            discover_modules(self.root / 'Modules')


class CommentPreservingEdits(Fixture):
    def test_adding_an_instance_keeps_comments(self):
        self.module('C', 'class C { public: explicit C(int n = 1) {} };')
        path = self.write('User/xrobot.yaml', '# robot config\nmodules:\n  - module: C  # first\n    id: c\n    args:\n      - n: 5  # tuned\n')
        append_module_instance('C', path, 'c2', self.root / 'Modules')
        text = path.read_text(encoding='utf-8')
        for comment in ('# robot config', '# first', '# tuned'):
            self.assertIn(comment, text)
        self.assertIn('id: c2', text)


if __name__ == '__main__':
    unittest.main()
