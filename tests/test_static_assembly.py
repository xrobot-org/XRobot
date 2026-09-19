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

    def test_defaults_explicit_and_saved_values_preserved(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_module('Foo', constructor_args=['int n = 42'], output_dir=root / 'Modules')
            config = root / 'User/xrobot.yaml'
            append_module_instance('Foo', config, modules_dir=root / 'Modules')
            self.assertEqual(load_config(config)['modules'], [{'module': 'Foo', 'id': 'foo_0', 'args': [{'n': '42'}]}])
            append_module_instance('Foo', config, modules_dir=root / 'Modules')
            self.assertEqual(load_config(config)['modules'][1]['id'], 'foo_1')
            self.assertEqual(load_config(config)['modules'][0]['args'], [{'n': '42'}])
            data = yaml.safe_load(config.read_text())
            data['modules'][0]['id'] = 'custom'
            config.write_text(yaml.safe_dump(data))
            append_module_instance('Foo', config, modules_dir=root / 'Modules')
            self.assertEqual(load_config(config)['modules'][0]['id'], 'custom')

    def test_creator_keeps_thin_metadata_and_generates_compile_only_ci(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaisesRegex(ValueError, 'owner/repo'):
                create_module('Bad', depends=['ShortName'], output_dir=root)
            self.assertFalse((root/'Bad').exists())
            path = create_module('Foo', depends=['team/Bar@2026-09-15'], output_dir=root)
            self.assertFalse((path/'tests').exists())
            workflow = (path/'.github/workflows/build.yml').read_text()
            self.assertIn('generate_compile_check', workflow)
            self.assertIn("'priority': -100", workflow)
            self.assertNotIn('createRef', workflow)
            self.assertIn('add_library(module_check OBJECT', workflow)
            self.assertNotIn('createRef', workflow)
            self.assertNotIn('XRobotMain(hw)', workflow)
            self.assertNotIn('HardwareContainer', (path/'Foo.hpp').read_text())


    def test_automatic_ids_separate_class_digits_from_sequence(self):
        self.assertEqual(get_next_instance_id([], 'DR16'), 'dr16_0')
        self.assertEqual(get_next_instance_id([], 'CMD'), 'cmd_0')
        self.assertEqual(get_next_instance_id([], 'RMMotor'), 'rmmotor_0')
        entries = [{'id': 'dr160'}, {'id': 'receiver'}, {'id': 'dr16_0'}, {'id': 'dr16_2'}]
        original = [dict(item) for item in entries]
        self.assertEqual(get_next_instance_id(entries, 'DR16'), 'dr16_1')
        self.assertEqual(entries, original)

    def test_adding_instance_keeps_existing_user_ids_and_bindings(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path = create_module('Foo', constructor_args=['int n=42'], output_dir=root/'Modules')
            config = root/'User/xrobot.yaml'
            config.parent.mkdir()
            config.write_text('modules:\n- module: Foo\n  id: foo0\n  args: [{n: "custom::foo0"}]\n')
            append_module_instance('Foo', config, modules_dir=root/'Modules')
            data = load_config(config)
            self.assertEqual(data['modules'][0]['id'], 'foo0')
            self.assertEqual(data['modules'][0]['args'], [{'n': 'custom::foo0'}])
            self.assertEqual(data['modules'][1]['id'], 'foo_0')
            append_module_instance('Foo', config, instance_id='my_receiver', modules_dir=root/'Modules')
            self.assertEqual(load_config(config)['modules'][2]['id'], 'my_receiver')

    def test_yaml_preserves_cpp_scalar_spelling(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'xrobot.yaml'
            path.write_text('modules:\n- module: Foo\n  id: foo0\n  args: [{a: on}, {b: off}, {c: yes}, {d: no}, {e: 0xFF}, {f: 001}, {g: 1e-3}, {h: true}, {i: \'"hello"\'}]\n')
            self.assertEqual(load_config(path)['modules'][0]['args'], [{k: v} for k, v in zip('abcdefghi', ['on', 'off', 'yes', 'no', '0xFF', '001', '1e-3', 'true', '"hello"'])])
            path.write_text('modules: []\nmodules: []\n')
            with self.assertRaisesRegex(ValueError, 'Duplicate'):
                load_config(path)

    def test_legacy_config_and_unnamed_arguments_rejected(self):
        invalid = [
            {'global_settings': {}, 'modules': []},
            {'modules': [{'name': 'Foo', 'id': 'f', 'constructor_args': {}}]},
            {'modules': [{'module': 'Foo', 'id': 'f', 'args': [{'a': 2, 'b': 3}]}]},
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
        self.standard = os.environ.get('XR_CXX_STANDARD', 'c++20')
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
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=90)
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
        self.generate([{'module':'Probe','id':'probe0','args':[{'right':'device'},{'pointer':'pointer'},{'configuration':'configuration'},{'flags':'flags'},{'fixed':'fixed'}]}], main)
        # The project CI image includes GCC's UBSan runtime but not clang's
        # compiler-rt UBSan archive. Runtime assertions still exercise the same
        # pointer/cv/base-adjustment contract under clang; add UBSan where the
        # selected toolchain has the runtime available by default.
        sanitizer = [] if os.name == 'nt' or 'clang' in Path(self.cxx).name else [
            '-fsanitize=undefined', '-fno-sanitize-recover=all']
        self.compile(extra=sanitizer)

    def test_local_registered_type_and_no_runtime_metadata(self):
        self.module('Accept', 'template<class T> class Accept { public: explicit Accept(T& x) { x.value = 2; } };')
        main = '#include "xrobot_main.hpp"\nint main(){ struct Local{int value=1;}; Local dev; XR_REGISTER(dev, Local); XROBOT_MAIN(); }'
        code = self.generate([{'module':'Accept','id':'a','template_args':['std::remove_reference_t<decltype(dev)>'],'args':[{'x':'dev'}]}],main)
        for forbidden in ('HardwareContainer','ApplicationManager','std::function','typeid','dynamic_cast'):
            self.assertNotIn(forbidden,code)
        self.compile()

    def test_pointer_covariance_rejected(self):
        self.module('Probe','struct Base{}; struct Derived: Base{}; class Probe { public: Probe(Base*& pointer) { (void)pointer; } };')
        self.generate([{'module':'Probe','id':'p','args':[{'pointer':'ptr'}]}], '#include "Probe.hpp"\n#include "xrobot_main.hpp"\nint main(){Derived d; Derived* ptr=&d; XR_REGISTER(ptr, Base*); XROBOT_MAIN();}')
        self.compile(expected=False)

    def test_const_cannot_be_restored_as_mutable(self):
        self.module('Probe','class Probe { public: Probe(int& value) { (void)value; } };')
        self.generate([{'module':'Probe','id':'p','args':[{'value':'value'}]}], '#include "xrobot_main.hpp"\nint main(){const int value=1; XR_REGISTER(value, int); XROBOT_MAIN();}')
        self.compile(expected=False)

    def test_template_defaults_and_constructor_overload(self):
        self.module('Choose','''#include <initializer_list>
#include <cassert>
template <class T = int> class Choose { public:
  Choose() { static_assert(std::is_same<T,int>::value, "template default"); }
  static int& Number() { static int number=42; return number; }
  Choose(int& n) { assert(n == 42); }
  Choose(std::initializer_list<int> values) { (void)values; std::abort(); }
};''')
        code=self.generate([{'module':'Choose','id':'first'}, {'module':'Choose','id':'second','args':[{'n':'Choose<>::Number()'}]}])
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
class B { public: explicit B(A& a){(void)a;assert(stage++ == 1);} void OnMonitor() noexcept {assert(stage++ == 3);} };''')
        for name, monitor in [('NoMonitor',''),('Wrong','int OnMonitor(){ std::abort(); }'),('Defaulted','void OnMonitor(int = 0){ std::abort(); }'),('Static','static void OnMonitor(){ std::abort(); }'),('Overloaded','void OnMonitor(){ std::abort(); } void OnMonitor(int){ std::abort(); }')]:
            self.module(name, 'class %s { public: %s() {} %s };' % (name,name,monitor))
        entries=[{'module':'A','id':'a'}, {'module':'B','id':'b','args':[{'a':'a'}]}] + [{'module':name,'id':name.lower()+'0'} for name in ['NoMonitor','Wrong','Defaulted','Static','Overloaded']]
        self.generate(entries)
        self.compile()

    def test_forward_instance_fails_in_cpp_not_python(self):
        self.module('A','class A { public: A(int n=1){(void)n;} int Get(){return 1;} };')
        self.generate([{'module':'A','id':'a','args':[{'n':'b.Get()'}]},{'module':'A','id':'b','args':[{'n':'1'}]}])
        self.compile(expected=False)

    def test_line_comments_cannot_swallow_argument_separators(self):
        self.module('A', '#include <cassert>\nclass A { public: A(int left=1, int right=2) { assert(left == 1 && right == 2); } };')
        self.generate([{'module':'A','id':'a0','args':[{'left':'1 // keep this comment'},{'right':'2'}]}])
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
        self.module('A','class A { public: A(int& value){(void)value;} };')
        self.generate([{'module':'A','id':'a','args':[{'value':'v'}]}], '#include "xrobot_main.hpp"\nint main(){XR_REGISTER(v, int); int v=1; XROBOT_MAIN();}')
        self.compile(expected=False)

    def test_changed_registration_type_requires_regeneration(self):
        self.module('A','class A { public: A(int& value){(void)value;} };')
        self.generate([{'module':'A','id':'a','args':[{'value':'v'}]}], '#include "xrobot_main.hpp"\nint main(){int v=1; XR_REGISTER(v, int); XROBOT_MAIN();}')
        source=self.root/'User/app_main.cpp'
        source.write_text(source.read_text().replace('XR_REGISTER(v, int)','XR_REGISTER(v, const int)'))
        self.compile(expected=False)


    def test_entry_is_global_explicit_and_macro_only_binds_names(self):
        self.module('Probe', 'struct Port {}; class Probe { public: Probe(Port& port) {(void)port;} void OnMonitor() {} };')
        code = self.generate([{'module':'Probe','id':'probe_0','args':[{'port':'port'}]}],
            '#include "xrobot_main.hpp"\nint main(){static Port port; XR_REGISTER(port, Port); XROBOT_MAIN();}')
        self.assertIn('void XRobotMain(\n    Port& port)', code)
        self.assertIn('#define XROBOT_MAIN() ::XRobotMain(port)', code)
        for absent in ('void Main(', 'XrView0', 'xr_slots', 'Erase(', 'XRobotMonitorAll', '__INTELLISENSE__', '__clangd__'):
            self.assertNotIn(absent, code)
        self.assertIn('::xrobot_generated::Monitor(probe_0);', code)
        self.compile()
        source = self.root/'User/app_main.cpp'
        source.write_text(source.read_text().replace('XROBOT_MAIN();','::XRobotMain(port);'))
        self.compile()

    def test_used_view_ignores_unused_alias_and_keeps_all_checks(self):
        self.module('Probe', 'struct Port {}; class Probe { public: Probe(Port& port) {(void)port;} };')
        code = self.generate([{'module':'Probe','id':'p','args':[{'port':'port'}]}],
            '#include "xrobot_main.hpp"\nint main(){struct Unused {}; static Unused unused; static Port port; XR_REGISTER(unused, Unused); XR_REGISTER(port, Port); XROBOT_MAIN();}')
        self.assertNotIn('template <typename xr_view_type_', code)
        self.assertIn('XR_REGISTER_DETAIL_unused',code)
        self.assertIn('::XRobotMain(port)',code)
        self.compile()

    def test_local_view_alias_keeps_the_declared_base_not_concrete_source(self):
        self.module('Probe', '''#include <cassert>
struct Port { int Value() const {return 1;} }; struct Device:Port {int Value() const {return 2;}};
template<class T> class Probe {public: Probe(T& port) {assert(port.Value()==1);} };''')
        code = self.generate([{'module':'Probe','id':'p','template_args':['std::remove_reference_t<decltype(port)>'],'args':[{'port':'port'}]}],
            '#include "xrobot_main.hpp"\nint main(){using LocalView=Port; static Device port; XR_REGISTER(port, LocalView); XROBOT_MAIN();}')
        self.assertIn('::XRobotMain<LocalView>(port)',code)
        self.compile()

    def test_multiple_local_views_and_global_view_keep_parameter_order(self):
        self.module('Probe', '''#include <cassert>
struct A{int a=3;}; struct B{int b=5;}; struct Device:A,B{};
class Probe {public: Probe(A& a,B& b,int*& ptr){assert(a.a==3&&b.b==5&&*ptr==7);} };''')
        self.generate([{'module':'Probe','id':'p','args':[{'a':'xr_view_dev_0'},{'b':'xr_view_dev_1'},{'ptr':'ptr'}]}],
            '#include "xrobot_main.hpp"\nint main(){using First=A; using Second=B; static Device dev; static int value=7; static int* ptr=&value; XR_REGISTER(ptr,int*); XR_REGISTER(dev,First,Second); XROBOT_MAIN();}')
        self.compile()

    def test_registered_array_and_function_pointer_storage(self):
        self.module('Probe', '''#include <cassert>
using Array = int[2]; using Callback = int(*)(int);
class Probe {public: Probe(Array& values,Callback& fn){assert(values[0]==3&&values[1]==5);assert(fn(4)==8); values[1]=9;} };''')
        self.generate([{'module':'Probe','id':'p','args':[{'values':'values'},{'fn':'fn'}]}],
            '#include "xrobot_main.hpp"\nint Twice(int x){return x*2;}\nint main(){static Array values={3,5}; static Callback fn=Twice; XR_REGISTER(values,Array); XR_REGISTER(fn,Callback); XROBOT_MAIN();}')
        self.compile()

    def test_registered_type_name_can_equal_the_object_name(self):
        self.module('Probe', 'struct Device {}; class Probe {public: Probe(Device& dev) {(void)dev;} };')
        code = self.generate([{'module':'Probe','id':'p','args':[{'dev':'Device'}]}],
            '#include "xrobot_main.hpp"\nint main(){static struct Device Device; XR_REGISTER(Device, struct Device); XROBOT_MAIN();}')
        self.assertIn('xr_view_Device_0', code)
        self.compile()

    def test_reference_entry_allows_distinct_multiple_and_virtual_bases(self):
        self.module('Probe', '''#include <cassert>
struct Port { virtual int Read(){return 1;} }; struct Left:virtual Port {}; struct Right {int value=7;};
struct Device:Left,Right {int Read() override{return 11;}};
class Probe {public: Probe(Port& p,Right& r){assert(p.Read()==11);assert(r.value==7);} };''')
        self.generate([{'module':'Probe','id':'p','args':[{'p':'dev'},{'r':'dev'}]}],
            '#include "xrobot_main.hpp"\nint main(){static Device dev; XR_REGISTER(dev,Port,Right); XROBOT_MAIN();}')
        self.compile()

    def test_ambiguous_base_view_is_rejected(self):
        self.module('Probe','struct Port {}; struct A:Port {}; struct B:Port {}; struct Device:A,B {}; class Probe { public: Probe(Port& p){(void)p;} };')
        self.generate([{'module':'Probe','id':'p','args':[{'p':'dev'}]}],
            '#include "xrobot_main.hpp"\nint main(){static Device dev; XR_REGISTER(dev,Port); XROBOT_MAIN();}')
        self.compile(expected=False)

    def test_include_header_without_call_still_checks_constructor(self):
        self.module('Probe','class Probe {public: Probe(int value){(void)value;} };')
        self.generate([{'module':'Probe','id':'p','args':[{'value':'"wrong"'}]}],
            '#include "xrobot_main.hpp"\nint main(){return 0;}')
        self.compile(expected=False)

    def test_header_is_self_contained_and_does_not_rewrite_unchanged_output(self):
        self.module('Probe','class Probe {public: Probe(int value=3){(void)value;} };')
        self.generate([{'module':'Probe','id':'p','args':[{'value':'3'}]}])
        header=self.root/'User/xrobot_main.hpp';stamp=header.stat().st_mtime_ns
        generate(self.root/'User/xrobot.yaml',self.root/'Modules',header)
        self.assertEqual(header.stat().st_mtime_ns,stamp)
        self.write('User/app_main.cpp','#include "xrobot_main.hpp"\n#include "xrobot_main.hpp"\nint main(){return 0;}')
        self.compile()

    def test_inline_attribute_follows_the_real_compile_profile(self):
        self.module('Probe','class Probe {public: Probe() {} };')
        self.generate([{'module':'Probe','id':'p'}])
        for flags, optimized in [(['-O0'],False),(['-O2'],False),(['-O2','-DNDEBUG'],True),(['-O2','-DNDEBUG','-DLIBXR_DEBUG_BUILD'],False),(['-O2','-DXROBOT_OPTIMIZED_BUILD=1'],True),(['-O2','-DNDEBUG','-DXROBOT_OPTIMIZED_BUILD=0'],False)]:
            with self.subTest(flags=flags):
                cmd=[self.cxx,'-std=c++20','-E','-P',*flags,'-I'+str(self.root),'-I'+str(self.root/'User'),'-I'+str(self.root/'Modules/Probe'),str(self.root/'User/app_main.cpp')]
                result=subprocess.run(cmd,capture_output=True,text=True,timeout=20)
                self.assertEqual(result.returncode,0,result.stderr)
                declaration=next(line for line in result.stdout.splitlines() if 'void XRobotMain(' in line)
                self.assertEqual('always_inline' in declaration, optimized and 'clang' in Path(self.cxx).name)
                self.compile(extra=flags)


    def test_view_declared_in_caller_after_generated_header(self):
        self.module('Probe', 'template<class T> class Probe {public: Probe(T& value){value.number=7;} };')
        self.generate([{'module':'Probe','id':'p','template_args':['std::remove_reference_t<decltype(dev)>'],'args':[{'value':'dev'}]}],
            '#include "xrobot_main.hpp"\nstruct View {int number=1;};\nint main(){static View dev; XR_REGISTER(dev,View); XROBOT_MAIN();}')
        self.compile()

    def test_caller_array_bound_from_enum_or_braced_constant(self):
        self.module('Probe', '#include <cassert>\ntemplate<class T> class Probe {public: Probe(T& value){assert(value.size()==2);} };')
        for declaration in ['enum {Count=2};', 'constexpr unsigned Count{2};']:
            with self.subTest(declaration=declaration):
                self.generate([{'module':'Probe','id':'p','template_args':['std::remove_reference_t<decltype(values)>'],'args':[{'value':'values'}]}],
                    '#include <array>\n#include "xrobot_main.hpp"\nint main(){'+declaration+'static std::array<int,Count> values{}; XR_REGISTER(values,std::array<int,Count>); XROBOT_MAIN();}')
                self.compile()

    def test_caller_function_pointer_typedef_keeps_storage(self):
        self.module('Probe', '#include <cassert>\ntemplate<class T> class Probe {public: Probe(T& fn){assert(fn(3)==6);} };')
        self.generate([{'module':'Probe','id':'p','template_args':['std::remove_reference_t<decltype(fn)>'],'args':[{'fn':'fn'}]}],
            '#include "xrobot_main.hpp"\nint Twice(int x){return x*2;}\nint main(){typedef int (*LocalCallback)(int); static LocalCallback fn=Twice; XR_REGISTER(fn,LocalCallback); XROBOT_MAIN();}')
        self.compile()

    def test_caller_non_type_template_argument_keeps_view(self):
        self.module('Probe', '#include <cassert>\ntemplate<class T> class Probe {public: Probe(T& values){assert(values.size()==2);} };')
        self.generate([{'module':'Probe','id':'p','template_args':['std::remove_reference_t<decltype(values)>'],'args':[{'values':'values'}]}],
            '#include <array>\n#include "xrobot_main.hpp"\ntemplate<unsigned Count> void Launch(){static std::array<int,Count> values{}; XR_REGISTER(values,std::array<int,Count>); XROBOT_MAIN();}\nint main(){Launch<2>();}')
        self.compile()


    def test_generated_template_parameters_cannot_shadow_user_names(self):
        self.module('Probe', 'template<class T> class Probe {public: Probe(T& value){value.number=7;} };')
        self.generate([{'module':'Probe','id':'XrView1','template_args':['std::remove_reference_t<decltype(XrView0)>'],'args':[{'value':'XrView0'}]}],
            '#include "xrobot_main.hpp"\nint main(){struct Local {int number=1;}; static Local XrView0; XR_REGISTER(XrView0,Local); XROBOT_MAIN();}')
        self.compile()


    def test_anonymous_local_typedef_preserves_registered_object(self):
        self.module('Probe', '#include <cassert>\ntemplate<class T> class Probe {public: Probe(T& value){assert(value.number==7);} };')
        self.generate([{'module':'Probe','id':'p','template_args':['std::remove_reference_t<decltype(dev)>'],'args':[{'value':'dev'}]}],
            '#include "xrobot_main.hpp"\nint main(){typedef struct {int number;} Local; static Local dev{7}; XR_REGISTER(dev,Local); XROBOT_MAIN();}')
        self.compile()


if __name__ == '__main__':
    unittest.main()
