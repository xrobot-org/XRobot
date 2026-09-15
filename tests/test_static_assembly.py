import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import yaml

from xrobot.CppSource import bind_identifiers, extract_interface, split_arguments, tokens
from xrobot.GenerateMain import generate, load_config, read_registrations, validate_config
from xrobot.ModuleParser import discover_modules, select_module
from xrobot.AddModule import append_module_instance, get_next_instance_id
from xrobot.ModuleCreator import create_module


class TextContracts(unittest.TestCase):
    def test_logical_tokens_not_substrings(self):
        bindings = {'dev': ['xr_view_dev_0']}
        text = 'f(dev, &dev, dev.member, obj.dev, ptr->dev, ns::dev, dev::constant, dev2, "dev", R"x(dev)x", \'d\' /*dev*/)'
        self.assertEqual(bind_identifiers(text, bindings), 'f(xr_view_dev_0, &xr_view_dev_0, xr_view_dev_0.member, obj.dev, ptr->dev, ns::dev, dev::constant, dev2, "dev", R"x(dev)x", \'d\' /*dev*/)')

    def test_multiview_ambiguity_is_explicit(self):
        with self.assertRaisesRegex(ValueError, 'xr_view_dev_0.*xr_view_dev_1'):
            bind_identifiers('dev.value', {'dev': ['xr_view_dev_0', 'xr_view_dev_1']})
        self.assertEqual(bind_identifiers('&xr_view_dev_1', {'dev': ['xr_view_dev_0', 'xr_view_dev_1']}), '&xr_view_dev_1')

    def test_literals_and_numbers(self):
        text = '0xff+dev+1e-3+u8"dev"+R"tag(\"dev\")tag"+dev'
        self.assertEqual(bind_identifiers(text, {'dev': ['bound']}), '0xff+bound+1e-3+u8"dev"+R"tag(\"dev\")tag"+bound')
        with self.assertRaises(ValueError):
            tokens('"unfinished')

    def test_declaration_extraction(self):
        code = '''// class Foo { Foo(int fake); };
        template <typename T = std::array<int, 2>, int N = 4>
        class Foo {
        public:
          explicit Foo(T value = T{1, 2}, const char* text = "),") : value_(value) {}
          Foo(int n) : value_{} { auto f = [](int x) { return x; }; f(n); }
          Foo(const Foo&) = delete;
        private:
          Foo(double);
          T value_;
        };'''
        interface = extract_interface(code, 'Foo')
        self.assertIn('std::array<int, 2>', interface['template'])
        self.assertEqual(len(interface['constructors']), 2)
        self.assertEqual(interface['constructors'][0]['parameters'], ['T value = T{1, 2}', 'const char* text = "),"'])

    def test_source_parser_rejects_unknown_interface(self):
        for text in ('namespace n { class Foo { public: Foo(); }; }', 'class Foo { public: DEFINE_CTOR(Foo); };', 'class Foo { public:\n#if FEATURE\nFoo(int);\n#endif\n};'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                extract_interface(text, 'Foo')

    def test_defaults_only_in_source(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_module('Foo', constructor_args=['int n = 42'], output_dir=root / 'Modules')
            config = root / 'User/xrobot.yaml'
            append_module_instance('Foo', config, modules_dir=root / 'Modules')
            self.assertEqual(load_config(config)['modules'], [{'module': 'Foo', 'id': 'foo0'}])
            append_module_instance('Foo', config, modules_dir=root / 'Modules')
            self.assertEqual(load_config(config)['modules'][1]['id'], 'foo1')
            self.assertNotIn('args', load_config(config)['modules'][0])
            data = yaml.safe_load(config.read_text())
            data['modules'][0]['id'] = 'custom'
            config.write_text(yaml.safe_dump(data))
            append_module_instance('Foo', config, modules_dir=root / 'Modules')
            self.assertEqual(load_config(config)['modules'][0]['id'], 'custom')

    def test_creator_keeps_thin_metadata_without_forced_module_tests(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaisesRegex(ValueError, 'owner/repo'):
                create_module('Bad', depends=['ShortName'], output_dir=root)
            self.assertFalse((root/'Bad').exists())
            path = create_module('Foo', depends=['team/Bar@2026-09-15'], output_dir=root)
            self.assertFalse((path/'tests').exists())
            self.assertFalse((path/'.github').exists())
            self.assertNotIn('HardwareContainer', (path/'Foo.hpp').read_text())


    def test_yaml_preserves_cpp_scalar_spelling(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'xrobot.yaml'
            path.write_text('modules:\n- module: Foo\n  id: foo0\n  args: [on, off, yes, no, 0xFF, 001, 1e-3, true, \'"hello"\']\n')
            self.assertEqual(load_config(path)['modules'][0]['args'], ['on', 'off', 'yes', 'no', '0xFF', '001', '1e-3', 'true', '"hello"'])
            path.write_text('modules: []\nmodules: []\n')
            with self.assertRaisesRegex(ValueError, 'Duplicate'):
                load_config(path)

    def test_legacy_config_and_aggregates_rejected(self):
        invalid = [
            {'global_settings': {}, 'modules': []},
            {'modules': [{'name': 'Foo', 'id': 'f', 'constructor_args': {}}]},
            {'modules': [{'module': 'Foo', 'id': 'f', 'args': [{'a': 2}]}]},
            {'modules': [{'module': 'Foo', 'id': 'f', 'args': [None]}]},
            {'modules': [{'module': 'Foo', 'id': 'f', 'args': ['@old']}]},
            {'modules': [{'module': 'Foo', 'id': 'f'}, {'module': 'Foo', 'id': 'f'}]},
        ]
        for config in invalid:
            with self.subTest(config=config), self.assertRaises(ValueError):
                validate_config(config)

    def test_registered_templates_and_empty_case(self):
        with tempfile.TemporaryDirectory() as temporary:
            p = Path(temporary) / 'main.cpp'
            p.write_text('XR_REGISTER(array, std::array<int, 2>);\n// XR_REGISTER(fake, Wrong);\n')
            self.assertEqual(read_registrations([p])[0]['types'], ['std::array<int, 2>'])
            p.write_text('XR_REGISTER(ref, Type&);')
            with self.assertRaisesRegex(ValueError, 'not reference'):
                read_registrations([p])

    def test_exported_nested_source_keeps_canonical_identity(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_module('Foo', output_dir=root / 'Modules/team')
            modules = discover_modules(root / 'Modules')
            self.assertEqual(select_module(modules, 'team/Foo')['id'], 'team/Foo')

    def test_same_name_package_requires_full_name(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            for owner in ('A', 'B'):
                create_module('Foo', output_dir=root / 'Modules' / owner)
            (root / 'xrobot.lock').write_text(yaml.safe_dump({'modules': {owner+'/Foo': {'directory': owner+'/Foo'} for owner in ('A', 'B')}}))
            packages = discover_modules(root / 'Modules')
            with self.assertRaisesRegex(ValueError, 'Ambiguous'):
                select_module(packages, 'Foo')
            self.assertEqual(select_module(packages, 'A/Foo')['id'], 'A/Foo')


@unittest.skipUnless(shutil.which(os.environ.get('CXX', 'g++')), 'C++ compiler not available on this host')
class GeneratedCpp(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / 'Modules').mkdir()
        (self.root / 'User').mkdir()
        self.cxx = os.environ.get('CXX', 'g++')
        self.standard = os.environ.get('XR_CXX_STANDARD', 'c++17')
        self.write('thread.hpp', '#pragma once\n#include <cstdlib>\nnamespace LibXR { struct Thread { static void Sleep(unsigned) { std::_Exit(0); } }; }\n')

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding='utf-8')

    def module(self, name, text):
        self.write('Modules/%s/%s.hpp' % (name, name), '#pragma once\n' + text)

    def generate(self, entries, main=None):
        if main is None:
            main = '#include "xrobot_main.hpp"\nint main(){ XROBOT_MAIN(); }\n'
        self.write('User/app_main.cpp', main)
        self.write('User/xrobot.yaml', yaml.safe_dump({'modules': entries}, sort_keys=False))
        return generate(self.root / 'User/xrobot.yaml', self.root / 'Modules', self.root / 'User/xrobot_main.hpp')

    def compile(self, expected=True, execute=True, extra=()):
        command = [self.cxx, '-std='+self.standard, '-Wall', '-Wextra', '-Werror', '-O2', '-I'+str(self.root), '-I'+str(self.root/'User')]
        command += ['-I'+str(p) for p in (self.root/'Modules').iterdir()]
        command += list(extra) + [str(self.root/'User/app_main.cpp'), '-o', str(self.root/'program')]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=45)
        if not expected:
            self.assertNotEqual(result.returncode, 0, result.stdout)
            return result.stdout
        self.assertEqual(result.returncode, 0, result.stdout)
        if execute:
            result = subprocess.run([str(self.root/'program')], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=8)
            self.assertEqual(result.returncode, 0, result.stdout)
        return result.stdout

    def test_cv_pointer_storage_base_adjustment_and_monitor_scope(self):
        self.write('thread.hpp', '''#pragma once
#include <cstdlib>
#include <cassert>
inline int constructors = 0, monitors = 0;
inline int alternate = 9;
inline int** expected_storage = nullptr;
inline const void* expected_right = nullptr;
namespace LibXR { struct Thread { static void Sleep(unsigned n) {
  assert(n == 1000); assert(constructors == 1); assert(*expected_storage == &alternate);
  if (monitors == 3) std::_Exit(0);
} }; }
''')
        self.module('Probe', '''#include "thread.hpp"
#include <cstdint>
struct Left { std::uintptr_t padding[4]{}; };
struct Right { int value = 17; };
struct Device : Left, Right {};
class Probe {
 public:
  Probe(Right& right, int*& pointer, const int& configuration,
        volatile int& flags, const int* const& fixed) {
    assert(static_cast<const void*>(&right) == expected_right);
    assert(&pointer == expected_storage); assert(configuration == 4);
    assert(*fixed == 4); assert(flags == 5); flags = 6;
    pointer = &alternate; ++constructors;
  }
  ~Probe() { std::abort(); }
  void OnMonitor() { ++monitors; }
};
''')
        main = '''#include "Probe.hpp"
#include "xrobot_main.hpp"
int main() {
  Device device; int value = 3; int* pointer = &value;
  const int configuration = 4; volatile int flags = 5;
  const int* const fixed = &configuration;
  expected_storage = &pointer;
  expected_right = static_cast<Right*>(&device);
  assert(expected_right != static_cast<void*>(&device));
  XR_REGISTER(device, Left, Right);
  XR_REGISTER(pointer, int*);
  XR_REGISTER(configuration, const int);
  XR_REGISTER(flags, volatile int);
  XR_REGISTER(fixed, const int* const);
  XROBOT_MAIN();
}
'''
        self.generate([{'module':'Probe','id':'probe0','args':['xr_view_device_1','pointer','configuration','flags','fixed']}], main)
        self.compile(extra=['-fsanitize=undefined', '-fno-sanitize-recover=all'])

    def test_local_registered_type_and_no_runtime_metadata(self):
        self.module('Accept', 'template<class T> class Accept { public: explicit Accept(T& x) { x.value = 2; } };')
        main = '#include "xrobot_main.hpp"\nint main(){ struct Local{int value=1;}; Local dev; XR_REGISTER(dev, Local); XROBOT_MAIN(); }'
        code = self.generate([{'module':'Accept','id':'a','template_args':['std::remove_reference_t<decltype(dev)>'],'args':['dev']}],main)
        for forbidden in ('HardwareContainer','ApplicationManager','std::function','typeid','dynamic_cast'):
            self.assertNotIn(forbidden,code)
        self.compile()

    def test_pointer_covariance_rejected(self):
        self.module('Probe','struct Base{}; struct Derived: Base{}; class Probe { public: Probe(Base*&) {} };')
        self.generate([{'module':'Probe','id':'p','args':['ptr']}], '#include "Probe.hpp"\n#include "xrobot_main.hpp"\nint main(){Derived d; Derived* ptr=&d; XR_REGISTER(ptr, Base*); XROBOT_MAIN();}')
        self.compile(expected=False)

    def test_const_cannot_be_restored_as_mutable(self):
        self.module('Probe','class Probe { public: Probe(int&) {} };')
        self.generate([{'module':'Probe','id':'p','args':['value']}], '#include "xrobot_main.hpp"\nint main(){const int value=1; XR_REGISTER(value, int); XROBOT_MAIN();}')
        self.compile(expected=False)

    def test_template_defaults_and_constructor_overload(self):
        self.module('Choose','''#include <initializer_list>
#include <cassert>
template <class T = int> class Choose { public:
  Choose() { static_assert(std::is_same<T,int>::value, "template default"); }
  Choose(int n) { assert(n == 42); }
  Choose(std::initializer_list<int>) { std::abort(); }
};''')
        code=self.generate([{'module':'Choose','id':'first'}, {'module':'Choose','id':'second','args':['42']}])
        self.assertIn('Choose<> first;',code)
        self.compile()

    def test_order_and_exact_optional_monitor(self):
        self.write('thread.hpp','''#pragma once
#include <cstdlib>
#include <cassert>
inline int stage=0;
namespace LibXR { struct Thread { static void Sleep(unsigned) { assert(stage == 4); std::_Exit(0); } }; }
''')
        self.module('A','''#include "thread.hpp"
class A { public: A(){assert(stage++ == 0);} void OnMonitor(){assert(stage++ == 2);} };''')
        self.module('B','''#include "thread.hpp"
class B { public: explicit B(A&){assert(stage++ == 1);} void OnMonitor() noexcept {assert(stage++ == 3);} };''')
        for name, monitor in [('NoMonitor',''),('Wrong','int OnMonitor(){ std::abort(); }'),('Defaulted','void OnMonitor(int = 0){ std::abort(); }'),('Static','static void OnMonitor(){ std::abort(); }'),('Overloaded','void OnMonitor(){ std::abort(); } void OnMonitor(int){ std::abort(); }')]:
            self.module(name, 'class %s { public: %s() {} %s };' % (name,name,monitor))
        entries=[{'module':'A','id':'a'}, {'module':'B','id':'b','args':['a']}] + [{'module':name,'id':name.lower()+'0'} for name in ['NoMonitor','Wrong','Defaulted','Static','Overloaded']]
        self.generate(entries)
        self.compile()

    def test_forward_instance_fails_in_cpp_not_python(self):
        self.module('A','class A { public: A(int){} int Get(){return 1;} };')
        self.generate([{'module':'A','id':'a','args':['b.Get()']},{'module':'A','id':'b','args':['1']}])
        self.compile(expected=False)

    def test_line_comments_cannot_swallow_argument_separators(self):
        self.module('A', '#include <cassert>\nclass A { public: A(int left, int right) { assert(left == 1 && right == 2); } };')
        self.generate([{'module':'A','id':'a0','args':['1 // keep this comment','2']}])
        self.compile()

    def test_no_instances_no_registration(self):
        self.generate([])
        self.compile(extra=['-pedantic-errors'])

    def test_generation_is_atomic_on_error(self):
        self.generate([])
        output=self.root/'User/xrobot_main.hpp'
        old=output.read_bytes()
        self.write('User/xrobot.yaml','modules:\n- module: Missing\n  id: missing0\n')
        with self.assertRaises(ValueError):
            generate(self.root/'User/xrobot.yaml',self.root/'Modules',output)
        self.assertEqual(output.read_bytes(),old)

    def test_registration_needs_a_visible_source_name(self):
        self.module('A','class A { public: A(int&){} };')
        self.generate([{'module':'A','id':'a','args':['v']}], '#include "xrobot_main.hpp"\nint main(){XR_REGISTER(v, int); int v=1; XROBOT_MAIN();}')
        self.compile(expected=False)

    def test_changed_registration_type_requires_regeneration(self):
        self.module('A','class A { public: A(int&){} };')
        self.generate([{'module':'A','id':'a','args':['v']}], '#include "xrobot_main.hpp"\nint main(){int v=1; XR_REGISTER(v, int); XROBOT_MAIN();}')
        source=self.root/'User/app_main.cpp'
        source.write_text(source.read_text().replace('XR_REGISTER(v, int)','XR_REGISTER(v, const int)'))
        self.compile(expected=False)


if __name__ == '__main__':
    unittest.main()
