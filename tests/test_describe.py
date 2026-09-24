"""xrobot_describe output and the xrobot_instance edits editors drive (R15)."""
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest
import yaml

from xrobot.AddModule import set_module_instance, remove_module_instance
from xrobot.Describe import describe
from xrobot.GenerateMain import generate
from test_review_rules import Fixture

MODULES = {
    'Led': 'class Led { public:\n'
           '  struct Param { int cycle; bool inverted; };\n'
           '  Led(LibXR::GPIO& gpio, const Param& param = {.cycle = 250, .inverted = false}) {}\n'
           '  void OnMonitor() {} };',
    'Motor': 'class Motor { public: Motor() {} virtual ~Motor() = default; };',
    'Wheel': '#include "Motor.hpp"\nclass Wheel : public Motor { public:\n'
             '  struct Config { struct Pid { float k; } pid; int id; };\n'
             '  class Limits { public: Limits(float max_speed, float max_current) {} };\n'
             '  explicit Wheel(Config config = {}, Limits limits = Limits(1.0F, 2.0F)) {} };',
    'Base': '#include "Motor.hpp"\nclass Base { public: explicit Base(Motor& motor) {} };',
}
MAIN = ('#include "xrobot_main.hpp"\nvoid app_main() {\n'
        '  XR_REGISTER(led_pin, LibXR::GPIO);\n  XR_REGISTER(uart1, LibXR::UART);\n}\n')


class Describe(Fixture):
    def setUp(self):
        super().setUp()
        for name, text in MODULES.items():
            self.module(name, text)
        self.write('User/app_main.cpp', MAIN)
        self.config = self.write('User/xrobot.yaml', yaml.safe_dump({'modules': [
            {'module': 'Led', 'id': 'led', 'args': [{'gpio': 'led_pin'}, {'param': {'cycle': 100, 'inverted': 'true'}}]},
            {'module': 'Wheel', 'id': 'wheel', 'args': [
                {'config': {'pid': {'k': '1.0F'}, 'id': 1}},
                {'limits': {'max_speed': '3.0F', 'max_current': '4.0F'}}]},
            {'module': 'Base', 'id': 'base', 'args': [{'motor': 'wheel'}]},
        ]}, sort_keys=False))

    def test_signatures_shapes_and_candidates(self):
        result = describe(self.root)
        self.assertEqual(result['diagnostics'], [])
        result['modules'] = {m['class']: m for m in result['modules'].values()}
        self.assertEqual(result['registrations'], [{'name': 'led_pin', 'types': ['LibXR::GPIO']},
                                                   {'name': 'uart1', 'types': ['LibXR::UART']}])
        led = result['modules']['Led']
        gpio, param = led['constructors'][0]['parameters']
        self.assertEqual(gpio['candidates'], ['led_pin'])
        self.assertEqual(param['default_fields'], {'cycle': '250', 'inverted': 'false'})
        wheel = result['modules']['Wheel']['constructors'][0]['parameters']
        self.assertEqual(result['types'][wheel[0]['type_ref']]['kind'], 'aggregate')
        pid = result['types'][wheel[0]['type_ref']]['fields'][0]
        self.assertEqual((pid['name'], result['types'][pid['type_ref']]['fields'][0]['name']), ('pid', 'k'))
        limits = result['types'][wheel[1]['type_ref']]
        self.assertEqual(limits['kind'], 'class')
        self.assertEqual([p['name'] for p in limits['constructors'][0]], ['max_speed', 'max_current'])
        # A derived instance is offered for a base-class reference.
        self.assertEqual(result['modules']['Base']['constructors'][0]['parameters'][0]['candidates'], ['wheel'])
        self.assertEqual([i['id'] for i in result['instances']], ['led', 'wheel', 'base'])

    def test_generation_errors_and_stamp_freshness_are_reported(self):
        self.assertEqual(describe(self.root)['entry']['status'], 'missing')
        generate(self.config, self.root / 'Modules', self.root / 'User/xrobot_main.hpp',
                 [self.root / 'User/app_main.cpp'])
        self.assertEqual(describe(self.root)['entry']['status'], 'fresh')
        set_module_instance('led', {'args': [{'gpio': 'led_pin'}, {'param': {'cycle': 1}}]}, self.config)
        result = describe(self.root)
        self.assertEqual(result['entry']['status'], 'stale')
        messages = [d['message'] for d in result['diagnostics']]
        self.assertTrue(any('led.args.param: missing inverted' in m for m in messages), messages)

    def test_lock_mismatch_is_reported_without_reading_that_module(self):
        locked = {name: {} for name in list(MODULES) + ['Gone']}
        self.write('xrobot.lock', yaml.safe_dump({'version': 1, 'modules': locked}))
        result = describe(self.root)
        self.assertEqual(result['lock']['status'], 'missing')
        self.assertNotIn('Gone', result['modules'])
        self.assertTrue(any('Gone from xrobot.lock is not checked out' in d['message'] for d in result['diagnostics']))

    def test_cli_prints_json(self):
        env = dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path))
        output = subprocess.run([sys.executable, '-m', 'xrobot.Describe', '-C', str(self.root)],
                                check=True, stdout=subprocess.PIPE, env=env).stdout
        self.assertEqual(json.loads(output)['schema'], 1)


class InstanceEdits(Fixture):
    def setUp(self):
        super().setUp()
        self.module('C', 'class C { public: explicit C(int n = 1) {} };')
        self.path = self.write('User/xrobot.yaml', '# robot\nmodules:\n  - module: C  # first\n    id: a\n'
                                                   '    args:\n      - n: 5  # tuned\n  - module: C\n    id: b\n')

    def test_set_and_remove_keep_other_entries_and_comments(self):
        set_module_instance('b', {'id': 'c', 'args': [{'n': '7'}]}, self.path)
        text = self.path.read_text(encoding='utf-8')
        for kept in ('# robot', '# first', '# tuned', 'id: c', "n: '7'"):
            self.assertIn(kept, text)
        remove_module_instance('c', self.path)
        text = self.path.read_text(encoding='utf-8')
        self.assertNotIn('id: c', text)
        self.assertIn('# tuned', text)

    def test_invalid_edits_leave_the_file_untouched(self):
        before = self.path.read_text(encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'Duplicate instance id'):
            set_module_instance('b', {'id': 'a'}, self.path)
        with self.assertRaisesRegex(ValueError, 'Only id, template_args and args'):
            set_module_instance('b', {'module': 'D'}, self.path)
        with self.assertRaisesRegex(ValueError, 'No module instance with id z'):
            remove_module_instance('z', self.path)
        self.assertEqual(self.path.read_text(encoding='utf-8'), before)


if __name__ == '__main__':
    unittest.main()
