import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import yaml

from xrobot.SourceSyntax import extract_interface
from xrobot.ConstructorModel import (enrich_interface, initial_arguments, initializer_tree,
    parameter, qualify, template_bindings, view_conversion, bind_value, constructor_for,
    type_shape, compliant_constructors)
from xrobot.GenerateMain import (generate, generate_compile_check, load_config,
    generate_xrobot_main_code, validate_config)
from xrobot.ModuleParser import discover_modules
from xrobot.AddModule import append_module_instance


def interface(source, name='Foo'):
    return enrich_interface(source, extract_interface(source, name))


class ConstructorContracts(unittest.TestCase):
    def test_named_parameters_and_raw_defaults(self):
        p = parameter('const std::array<float, 2>& gains = {1.0f, 0.0f}')
        self.assertEqual((p['name'], p['type'], p['default']),
            ('gains', 'const std::array<float, 2>&', '{1.0f, 0.0f}'))
        with self.assertRaisesRegex(ValueError, 'explicit names'):
            parameter('LibXR::UART&')

    def test_only_explicit_initializer_tree_is_expanded(self):
        source = '''class Foo { public:
          struct Param { int hidden = 99; };
          Foo(Param p = {.hidden = 12}) {}
        };'''
        self.assertEqual(initial_arguments(interface(source)), [{'p': {'hidden': '12'}}])
        self.assertEqual(initializer_tree('{}'), [])
        self.assertEqual(initializer_tree('Foo::Factory(1, 2)'), 'Foo::Factory(1, 2)')
        self.assertEqual(initializer_tree('Param{.gain={1,2},.flag=true}', 'const Param&'),
                         {'gain': ['1', '2'], 'flag': 'true'})

    def test_public_defaults_qualified_private_defaults_unfilled(self):
        d = interface('''class Foo { private: static int Secret();
          public: static int Factory(); Foo(int x=Factory(), int y=Secret()) {} };''')
        diagnostics = []
        self.assertEqual(initial_arguments(d, diagnostics=diagnostics), [{'x': 'Foo::Factory()'}, {'y': None}])
        self.assertIn('original default: Secret()', diagnostics[0])
        with self.assertRaisesRegex(ValueError, 'original default: Secret'):
            initial_arguments(d)

    def test_typedef_and_alias_type_names_are_not_reflected(self):
        d = interface('''class Foo { public: typedef struct { int n; } Param;
          using Options = Param; Foo(const Options& p) {} };''')
        self.assertEqual(qualify('const Options&', d, 'Foo'), 'const Foo::Options&')
        self.assertEqual(qualify('const Options&', d, 'Foo', expand_aliases=True), 'const Foo::Param&')
        self.assertEqual(initial_arguments(d), [{'p': None}])

    def test_scope_roots_are_qualified_but_unrelated_names_are_not(self):
        model = interface("class Foo { private: static int Secret(); public: enum class Mode {A}; Foo(Mode mode=Mode::A) {} };")
        self.assertEqual(initial_arguments(model), [{'mode': 'Foo::Mode::A'}])
        self.assertEqual(qualify('Vendor::Secret()', model, 'Foo'), 'Vendor::Secret()')
        self.assertEqual(qualify('{.Secret=4}', model, 'Foo'), '{.Secret=4}')
        with self.assertRaisesRegex(ValueError, 'non-public'):
            qualify('Foo::Secret()', model, 'Foo')
        generic = interface('template<class T> class Foo { public: Foo(typename T::Value value) {} };')
        self.assertEqual(qualify('typename T::Value', generic, 'Foo<Options>', {'T':'Options'}), 'typename Options::Value')

    def test_unknown_typed_initializers_and_lambdas_keep_their_semantics(self):
        self.assertEqual(initializer_tree('Derived{7}', 'Base'), 'Derived{7}')
        self.assertEqual(initializer_tree('[]{ return 7; }'), '[]{ return 7; }')
        self.assertEqual(initializer_tree('{.inner=Vendor::Options{7}}'), {'inner':'Vendor::Options{7}'})
        self.assertEqual(initializer_tree('Options{.count=7}', 'const Options&'), {'count':'7'})

    def test_same_named_overloads_use_explicit_types_not_guesswork(self):
        model = interface('class Foo { public: Foo(int value=0) {} Foo(float value=0.0f) {} };')
        for expression, expected in [('7','int'), ('1.0f','float'), ('static_cast<float>(Read())','float'), ('int{7}','int')]:
            selected = constructor_for(model, [{'value':expression}], {}, 'Foo', {})
            self.assertEqual(selected['arguments'][0]['type'], expected)
        with self.assertRaisesRegex(ValueError, 'ambiguous'):
            constructor_for(model, [{'value':'Read()'}], {}, 'Foo', {})

    def test_inaccessible_defaults_report_the_original_expression(self):
        model = interface('class Foo { static int Secret(); public: Foo(int value=Secret()) {} };')
        diagnostics = []
        self.assertEqual(initial_arguments(model, diagnostics=diagnostics), [{'value':None}])
        self.assertEqual(len(diagnostics), 1)
        self.assertIn('original default: Secret()', diagnostics[0])

    def test_template_values_require_real_compile_time_selection(self):
        d = interface('template<typename T, unsigned N = 4> class Foo {public: Foo(T x=T{}) {}};')
        self.assertEqual(template_bindings(d, ['float']), {'T': 'float', 'N': '4'})
        with self.assertRaisesRegex(ValueError, 'must be specified'):
            template_bindings(d, [])

    def test_view_address_and_pointer_storage_are_different(self):
        self.assertEqual(view_conversion('UART', 'UART&', 'v'), 'v')
        self.assertEqual(view_conversion('UART', 'UART*', 'v'), 'std::addressof(v)')
        self.assertEqual(view_conversion('UART*', 'UART*&', 'v'), 'v')
        self.assertEqual(view_conversion('UART*', 'UART* const&', 'v'), 'v')
        self.assertIsNone(view_conversion('const UART', 'UART&', 'v'))
        self.assertIsNone(view_conversion('UART*', 'const UART*&', 'v'))

    def test_direct_target_selects_view_and_preserves_complex_ambiguity(self):
        views = {'device': [('Left', 'l'), ('Right', 'r')]}
        binds = {'device': ['l', 'r']}
        self.assertEqual(bind_value('device', 'Right&', views, binds), 'r')
        self.assertEqual(bind_value('device', 'Right*', views, binds), 'std::addressof(r)')
        with self.assertRaisesRegex(ValueError, 'Ambiguous'):
            bind_value('Read(device)', 'int', views, binds)
        cv = {'device': [('Right', 'r'), ('const Right', 'cr')]}
        self.assertEqual(bind_value('device', 'const Right&', cv, binds), 'cr')

    def test_first_constructor_can_declare_a_dependency_without_default(self):
        model = interface('class Foo {public: Foo(int dependency); Foo(float gain=1.0f);};')
        self.assertEqual(initial_arguments(model), [{'dependency': None}])
        self.assertEqual(len(compliant_constructors(model)), 2)
        self.assertEqual(constructor_for(model, [{'gain':'1.0f'}], {}, 'Foo', {})['arguments'][0]['type'], 'float')

    def test_leading_no_default_parameter_is_dependency_by_contract(self):
        for declaration in ('GPIO& gpio', 'std::initializer_list<GPIO*> channels'):
            model = interface('class Foo {public: Foo(' + declaration + ');};')
            self.assertEqual(initial_arguments(model), [{declaration.split()[-1]: None}])

    def test_dependencies_must_precede_default_configuration(self):
        model = interface('class Foo {public: Foo(int count=10, GPIO& gpio);};')
        with self.assertRaisesRegex(ValueError, 'dependency without a default appears after'):
            initial_arguments(model)

    def test_template_contents_do_not_become_outer_cv_or_pointer_levels(self):
        base, cv, pointers, reference = type_shape('const std::array<const GPIO*, 2>&')
        self.assertEqual(base, 'std::array<constGPIO*,2>')
        self.assertEqual(cv, frozenset({'const'}))
        self.assertEqual(pointers, ())
        self.assertEqual(reference, '&')
        self.assertIsNone(view_conversion('std::array<GPIO*, 2>',
                                         'std::array<const GPIO*, 2>&', 'v'))

    def test_yaml_null_is_placeholder_not_cpp_nullptr(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary)/'config.yaml'
            path.write_text("modules:\n- module: Foo\n  id: f\n  args:\n  - required: null\n  - optional: nullptr\n  - text: 'null'\n", encoding='utf-8')
            values = load_config(path)['modules'][0]['args']
            self.assertEqual(values, [{'required': None}, {'optional': 'nullptr'}, {'text': 'null'}])
            validate_config({'modules': [{'module': 'Foo', 'id': 'f', 'args': [{'p': None}]}]})

    def test_first_constructor_defaults_do_not_overwrite_existing_config(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); folder=root/'Modules/team/Foo'; folder.mkdir(parents=True)
            header = folder/'Foo.hpp'
            header.write_text('class Foo {public: Foo(int period=20) {} Foo(float gain=1.0f) {}};')
            config=root/'User/config.yaml'
            append_module_instance('team/Foo', config, modules_dir=root/'Modules')
            self.assertEqual(load_config(config)['modules'][0]['args'], [{'period': '20'}])
            header.write_text('class Foo {public: Foo(int period=30) {} Foo(float gain=1.0f) {}};')
            append_module_instance('team/Foo', config, modules_dir=root/'Modules')
            self.assertEqual(load_config(config)['modules'][0]['args'], [{'period': '20'}])
            self.assertEqual(load_config(config)['modules'][1]['args'], [{'period': '30'}])
            modules=discover_modules(root/'Modules')
            code=generate_xrobot_main_code({'modules':[{'module':'team/Foo','id':'f','args':[{'gain':'1.0f'}]}]}, modules)
            self.assertIn('1.0f', code)
            self.assertNotIn('Argument<', code)
            with self.assertRaisesRegex(ValueError, 'do not match'):
                generate_xrobot_main_code({'modules':[{'module':'team/Foo','id':'f'}]}, modules)


@unittest.skipUnless(shutil.which(os.environ.get('CXX', 'g++')), 'C++ compiler not available')
class ConstructorCpp(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root=Path(self.temporary.name)
        (self.root/'Modules/team/Foo').mkdir(parents=True)
        (self.root/'User').mkdir()
        (self.root/'thread.hpp').write_text('#pragma once\n#include <cstdlib>\nnamespace LibXR {struct Thread {static void Sleep(unsigned) {std::_Exit(0);}};}\n')

    def build(self, source, execute=False):
        command=[os.environ.get('CXX','g++'),'-std=c++20','-Wall','-Wextra','-Werror','-O1',
          '-I'+str(self.root),'-I'+str(self.root/'User'),'-I'+str(self.root/'Modules/team/Foo')]
        output=self.root/('program' if execute else 'check.o')
        if not execute: command+=['-c']
        command+=[str(source),'-o',str(output)]
        result=subprocess.run(command,capture_output=True,text=True,timeout=45)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        if execute:
            result=subprocess.run([str(output)],capture_output=True,text=True,timeout=10)
            self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_static_config_lifetime_nested_values_and_pointer_binding(self):
        header=self.root/'Modules/team/Foo/Foo.hpp'
        header.write_text('''#pragma once
#include <cassert>
#include <initializer_list>
struct Left { long padding[3]{}; }; struct Right { int value = 17; };
struct Device : Left, Right {};
class Foo { public:
  struct Param { struct Gain { float kp, ki; }; Gain gain; int number; };
  Foo(Right& right, Right* optional, const Param& p=Param{}, std::initializer_list<int> values={7,8})
    : p_(p), values_(values) { assert(&right==optional); assert(right.value==17); }
  void OnMonitor() const noexcept { assert(p_.gain.kp==1 && p_.gain.ki==2); assert(p_.number==3); assert(*values_.begin()==7); }
private: const Param& p_; std::initializer_list<int> values_;
};''')
        config={'modules':[{'module':'team/Foo','id':'f','args':[
          {'right':'device'},{'optional':'device'},
          {'p':{'gain':{'kp':'1.0f','ki':'2.0f'},'number':'3'}},{'values':['7','8']}]}]}
        (self.root/'User/config.yaml').write_text(yaml.safe_dump(config,sort_keys=False))
        main=self.root/'User/app_main.cpp'
        main.write_text('#include "Foo.hpp"\n#include "xrobot_main.hpp"\nint main(){static Device device; XR_REGISTER(device, Left, Right); XROBOT_MAIN();}\n')
        code=generate(self.root/'User/config.yaml',self.root/'Modules',self.root/'User/xrobot_main.hpp',[main])
        self.assertIn('static const Foo::Param&',code)
        self.assertIn('static std::initializer_list<int>',code)
        self.build(main,execute=True)

    def test_compile_check_uses_void_pointer_and_is_not_linked_or_run(self):
        header=self.root/'Modules/team/Foo/Foo.hpp'
        header.write_text('''#pragma once
#include <cstdlib>
struct Uart { Uart() = delete; };
class Foo {public: Foo(Uart& uart, int period=10) { (void)uart; (void)period; std::abort(); }
void OnMonitor() { std::abort(); }};''')
        out=self.root/'check.cpp'
        code=generate_compile_check('team/Foo',self.root/'Modules',out)
        self.assertIn('static_cast<void*>(nullptr)',code)
        self.assertIn('static Foo module_0',code)
        self.assertNotIn('int main(',code)
        self.assertNotIn('for (;;)',code)
        self.build(out)

    def test_named_overload_is_not_reselected_by_literal_type(self):
        header = self.root/'Modules/team/Foo/Foo.hpp'
        header.write_text("""#pragma once
#include <cassert>
class Foo {public:
  Foo(int count=0) { (void)count; assert(false && "wrong constructor"); }
  Foo(float gain=0.0f) { assert(gain==1.0f); }
};""")
        config = self.root/'User/config.yaml'
        config.write_text('modules:\n- module: team/Foo\n  id: f\n  args:\n  - gain: 1\n')
        main = self.root/'User/app_main.cpp'
        main.write_text('#include "xrobot_main.hpp"\nint main(){XROBOT_MAIN();}\n')
        generate(config, self.root/'Modules', self.root/'User/xrobot_main.hpp')
        self.build(main, execute=True)

    def test_unused_registered_views_do_not_occupy_entry_parameters(self):
        header = self.root/'Modules/team/Foo/Foo.hpp'
        header.write_text('struct Device {}; class Foo {public: Foo(Device& device) {(void)device;} };')
        config = self.root/'User/config.yaml'
        config.write_text('modules:\n- module: team/Foo\n  id: foo_0\n  args: [{device: used}]\n')
        main = self.root/'User/app_main.cpp'
        main.write_text('#include "xrobot_main.hpp"\nint main(){static Device used, unused; XR_REGISTER(unused,Device); XR_REGISTER(used,Device); XROBOT_MAIN();}\n')
        code = generate(config, self.root/'Modules', self.root/'User/xrobot_main.hpp', [main])
        self.assertIn('Device& used', code)
        self.assertNotIn('std::array<void*', code)
        self.assertNotIn('Erase<Device>(unused)', code)
        self.assertIn('XR_REGISTER_DETAIL_unused', code)
        self.assertIn('#define XROBOT_MAIN() ::XRobotMain(used)', code)
        self.assertNotIn('xr_slots', code)
        self.build(main, execute=True)

    def test_named_dependency_has_no_redundant_static_reference_storage(self):
        header = self.root/'Modules/team/Foo/Foo.hpp'
        header.write_text('class Device {}; class Foo { public: Foo(Device& device) {(void)device;} };')
        config = self.root/'User/config.yaml'
        config.write_text('modules:\n- module: team/Foo\n  id: foo_0\n  args: [{device: device}]\n')
        main = self.root/'User/app_main.cpp'
        main.write_text('#include "xrobot_main.hpp"\nint main(){static Device device; XR_REGISTER(device, Device); XROBOT_MAIN();}\n')
        code = generate(config, self.root/'Modules', self.root/'User/xrobot_main.hpp', [main])
        self.assertNotIn('xr_arg_foo_0_device', code)
        self.assertIn('static_cast<Device&>', code)
        self.build(main, execute=True)

    def test_factory_prvalue_preserves_guaranteed_copy_elision(self):
        header = self.root/'Modules/team/Foo/Foo.hpp'
        header.write_text("""#pragma once
struct Config {
  Config() = default;
  Config(const Config&) = delete;
  Config(Config&&) = delete;
};
class Foo {public:
  static Config DefaultConfig() { return Config{}; }
  Foo(Config config=DefaultConfig()) { (void)config; }
  Foo(int count=0) { (void)count; }
};""")
        config = self.root/'User/config.yaml'
        config.write_text('modules:\n- module: team/Foo\n  id: f\n  args:\n  - config: Foo::DefaultConfig()\n')
        main = self.root/'User/app_main.cpp'
        main.write_text('#include "xrobot_main.hpp"\nint main(){XROBOT_MAIN();}\n')
        code = generate(config, self.root/'Modules', self.root/'User/xrobot_main.hpp')
        self.assertNotIn('Argument<', code)
        self.build(main, execute=True)

    def test_named_overload_does_not_enable_explicit_only_conversion(self):
        header = self.root/'Modules/team/Foo/Foo.hpp'
        header.write_text("""#pragma once
struct Config { explicit Config(int) {} };
class Foo {public:
  Foo(Config config=Config{0}) { (void)config; }
  Foo(int count=0) { (void)count; }
};""")
        config = self.root/'User/config.yaml'
        config.write_text('modules:\n- module: team/Foo\n  id: f\n  args:\n  - config: 1\n')
        main = self.root/'User/app_main.cpp'
        main.write_text('#include "xrobot_main.hpp"\nint main(){XROBOT_MAIN();}\n')
        generate(config, self.root/'Modules', self.root/'User/xrobot_main.hpp')
        command = [os.environ.get('CXX', 'g++'), '-std=c++20', '-c', str(main),
                   '-I'+str(self.root), '-I'+str(self.root/'User'),
                   '-I'+str(header.parent), '-o', str(self.root/'bad.o')]
        result = subprocess.run(command, capture_output=True, text=True, timeout=45)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('Named constructor argument requires an implicit conversion', result.stderr)

    def test_compile_check_rejects_inaccessible_value_defaults(self):
        header = self.root/'Modules/team/Foo/Foo.hpp'
        out = self.root/'check.cpp'
        out.write_text('original')
        for source in ('class Foo {static int Secret(); public: Foo(int count=Secret());};',):
            header.write_text(source)
            with self.assertRaises(ValueError):
                generate_compile_check('team/Foo', self.root/'Modules', out)
            self.assertEqual(out.read_text(), 'original')

    def test_missing_required_value_fails_before_output_changes(self):
        header=self.root/'Modules/team/Foo/Foo.hpp'
        header.write_text('class Foo {public: Foo(int period=10) {(void)period;}};')
        config=self.root/'User/config.yaml'
        config.write_text('modules:\n- module: team/Foo\n  id: f\n  args:\n  - period: null\n')
        out=self.root/'User/xrobot_main.hpp';out.write_text('original')
        with self.assertRaisesRegex(ValueError,'f.args.period is not filled in'):
            generate(config,self.root/'Modules',out)
        self.assertEqual(out.read_text(),'original')


if __name__ == '__main__':
    unittest.main()
