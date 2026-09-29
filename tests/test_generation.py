"""Generating User/xrobot_main.hpp: header shape, argument binding and conversion, dependency
rules, diagnostics, and C++ compile/run checks of the generated code."""
import os
import re
import stat
import unittest
from pathlib import Path

from fixtures import BspTestCase, CXX, CxxMixin, requires_cxx
from xrobot.Config import ConfigError
from xrobot.GenerateMain import generate, generate_compile_check, load_modules, validate_all

MAIN = '#include "xrobot_main.hpp"\nint main() { XROBOT_MAIN(); }\n'

LED = '''namespace LibXR { class GPIO; class UART; }
struct Port { int value = 1; };
struct SubPort : Port {};
struct Hidden : private Port {};
class Led {
 public:
  struct Param { int cycle = 250; bool inverted{}; };
  Led(LibXR::GPIO& gpio, Param param = {}, float gain = 1.0f) {}
  void OnMonitor() {}
};'''
PROBE = '''#include "Led.hpp"
class Probe {
 public:
  Probe(Port& port, Port* optional, int count = 1, const char* name = "probe") {}
};'''
CMD = '''#include "Led.hpp"
class Cmd {
 public:
  explicit Cmd(Led& led, Led* backup) {}
  void OnMonitor() const noexcept {}
};'''
REGISTERED = ('#include "xrobot_main.hpp"\nint main() {\n'
              '  XR_REGISTER(pin, LibXR::GPIO);\n  XR_REGISTER(uart, LibXR::UART);\n'
              '  XR_REGISTER(port, Port);\n  XR_REGISTER(sub, SubPort);\n'
              '  XR_REGISTER(hidden, Hidden);\n  XR_REGISTER(unused, int);\n  XROBOT_MAIN();\n}\n')


def led(identity='led', gpio='pin', param=None, gain='1.0f'):
    return {'module': 'Led', 'id': identity, 'args': [
        {'gpio': gpio}, {'param': param or {'cycle': '250', 'inverted': 'false'}}, {'gain': gain}]}


def probe(identity='probe', port='port', optional='nullptr', count='1', name='"probe"'):
    return {'module': 'Probe', 'id': identity, 'args': [
        {'port': port}, {'optional': optional}, {'count': count}, {'name': name}]}


class GenerationTestCase(BspTestCase):
    def setUp(self):
        super().setUp()
        self.module('Led', LED)
        self.module('Probe', PROBE)
        self.module('Cmd', CMD)
        self.entry(REGISTERED)

    def code(self, *instances, **top):
        top['modules'] = list(instances)
        return self.generate(top)

    def error(self, *instances, **top):
        top['modules'] = list(instances)
        with self.assertRaises(ValueError) as context:
            self.generate(top)
        return str(context.exception)


class HeaderShape(GenerationTestCase):
    def test_entry_function_takes_only_consumed_registrations(self):
        code = self.code(led())
        self.assertIn('[[noreturn]] static inline void XRobotMain(\n    LibXR::GPIO& pin)\n{', code)
        self.assertIn('\n#define XROBOT_MAIN() ::XRobotMain(pin)\n', code)
        for name in ('uart', 'unused', 'hidden', 'port'):
            self.assertNotRegex(code, r'\b%s\b' % name)

    def test_register_macro_expands_to_nothing(self):
        code = self.code(led())
        self.assertIn('\n#define XR_REGISTER(name, ...) static_cast<void>(name)\n', code)
        self.assertNotIn('XR_REGISTER_DETAIL', code)
        self.assertNotRegex(code, r'static_assert\([^;]*\bpin\b')

    def test_no_instances_give_a_parameterless_entry_that_still_sleeps(self):
        code = self.code()
        self.assertIn('[[noreturn]] static inline void XRobotMain()\n{', code)
        self.assertIn('#define XROBOT_MAIN() ::XRobotMain()\n', code)
        self.assertIn('    LibXR::Thread::Sleep(1000);', code)

    def test_includes_libxr_the_selected_module_headers_and_constexpr_headers(self):
        code = self.code(led(), constexpr_includes=['Board.hpp', '<vector>', 'sub/Pins.h'])
        includes = [line for line in code.splitlines() if line.startswith('#include')]
        self.assertEqual(includes, ['#include <memory>', '#include <type_traits>', '#include <utility>',
                                    '#include "libxr.hpp"', '#include "thread.hpp"', '#include "Led.hpp"',
                                    '#include "Board.hpp"', '#include <vector>', '#include "sub/Pins.h"'])

    def test_constexprs_are_emitted_in_their_namespace_before_the_entry(self):
        code = self.code(led(param={'cycle': 'Board::Rate', 'inverted': 'false'}),
                         constexpr_namespace='Board',
                         constexprs={'Rate': {'type': 'int', 'value': '250'},
                                     'Gains': {'type': 'std::array<float, 2>', 'value': '{1.0F, 2.0F}'}})
        self.assertIn('namespace Board {\ninline constexpr int Rate = 250;\n'
                      'inline constexpr std::array<float, 2> Gains = std::array<float, 2>{1.0F, 2.0F};\n'
                      '}  // namespace Board', code)
        self.assertLess(code.index('inline constexpr int Rate'), code.index('void XRobotMain('))
        self.assertIn('.cycle = Board::Rate', code)

    def test_constexpr_errors_are_reported(self):
        message = self.error(constexprs={'Rate': {'type': 'int', 'value': None}})
        self.assertRegex(message, r'xrobot\.yaml: constexprs\.Rate(\.value)? is not filled in')

    def test_monitor_sleep_is_taken_from_settings(self):
        self.assertIn('LibXR::Thread::Sleep(20);', self.code(settings={'monitor_sleep_ms': '20'}))

    def test_instances_are_function_local_statics_in_yaml_order(self):
        code = self.code(led('b_led'), led('a_led'), probe())
        order = [m.group(1) for m in re.finditer(r'^  static \w+ (\w+)\(', code, re.M)]
        self.assertEqual(order, ['b_led', 'a_led', 'probe'])

    def test_monitors_are_called_in_yaml_order_with_a_void_check(self):
        code = self.code(led('first'), {'module': 'Cmd', 'id': 'cmd', 'args': [{'led': 'first'}, {'backup': 'nullptr'}]},
                         probe(), led('last'))
        loop = code[code.index('for (;;)'):]
        self.assertEqual(re.findall(r'(\w+)\.OnMonitor\(\);', loop), ['first', 'cmd', 'last'])
        for identity in ('first', 'cmd', 'last'):
            self.assertIn('static_assert(std::is_void_v<decltype(%s.OnMonitor())>, "%s.OnMonitor() must return void");'
                          % (identity, identity), code)
        self.assertNotIn('probe.OnMonitor', code)


class LineDirectives(GenerationTestCase):
    def test_directives_map_instances_and_arguments_to_yaml_and_back_to_the_header(self):
        config = self.config('# robot\nmodules:\n  - module: Led\n    id: led\n    args:\n      - gpio: pin\n'
                             '      - param:\n          cycle: 100\n          inverted: "true"\n'
                             '      - gain: "2.5F"\n  - module: Probe\n    id: probe\n    args:\n'
                             '      - port: sub\n      - optional: "&port"\n      - count: 2\n'
                             '      - name: \'"p"\'\n')
        code = generate(self.project, config)
        header = (self.root / 'User/xrobot_main.hpp').resolve().as_posix()
        yaml_path = config.resolve().as_posix()
        yaml_targets = []
        lines = code.split('\n')
        for number, line in enumerate(lines, 1):
            match = re.fullmatch(r'#line (\d+) "(.*)"', line)
            if not match:
                continue
            target, path = int(match.group(1)), match.group(2)
            if path == header:
                self.assertEqual(target, number + 1, 'a #line back into the header must name the next line')
            else:
                self.assertEqual(path, yaml_path)
                yaml_targets.append((target, lines[number].strip()))
        # instance line, then one directive per argument; the header resumes after each instance.
        self.assertIn((3, 'static Led led('), yaml_targets)
        self.assertIn((6, 'pin'), yaml_targets)
        self.assertIn((7, ', std::remove_cv_t<std::remove_reference_t<Led::Param>>{'), yaml_targets)
        self.assertIn((10, ', xrobot_generated::Implicit<float>(2.5F)'), yaml_targets)
        self.assertIn((11, 'static Probe probe('), yaml_targets)
        self.assertIn((14, 'static_assert(std::is_same_v<std::remove_cvref_t<decltype((sub))>, '
                            'std::remove_cvref_t<Port&>> ||'), yaml_targets)
        self.assertIn((15, ', &port'), yaml_targets)
        back = [line for line in lines if line.endswith('"%s"' % header)]
        self.assertEqual(len(back), 2)

    def test_the_compile_check_probe_has_no_line_directives(self):
        out = self.tmp / 'check.cpp'
        code = generate_compile_check('team/Probe', load_modules(self.project), out)
        self.assertNotIn('#line', code)


class Conversions(GenerationTestCase):
    def test_arithmetic_parameters_use_implicit(self):
        code = self.code(led(gain='2'), probe(count='3'))
        self.assertIn('xrobot_generated::Implicit<float>(2)', code)
        self.assertIn('xrobot_generated::Implicit<int>(3)', code)
        self.assertNotIn('static_cast<int>', code)

    def test_other_values_get_an_implicit_conversion_check_and_a_cast(self):
        code = self.code(probe(name='"left"'))
        self.assertIn('std::is_convertible_v<decltype(("left")), const char*>,\n'
                      '              "probe.args.name requires an implicit conversion to const char*");', code)
        self.assertIn(', static_cast<const char*>("left")', code)

    def test_a_registration_of_the_parameter_type_binds_directly(self):
        code = self.code(probe(port='port'))
        self.assertIn('static Probe probe(\n', code)
        self.assertRegex(code, r'\n      port\n')
        self.assertNotIn('static_cast<Port&>(port)', code)

    def test_a_derived_registration_is_checked_and_cast(self):
        code = self.code(probe(port='sub'))
        self.assertIn('std::is_convertible_v<decltype((sub)), Port&>', code)
        self.assertIn('static_cast<Port&>(sub)', code)

    def test_a_bare_name_for_a_pointer_parameter_passes_its_address(self):
        code = self.code(probe(optional='port'))
        self.assertRegex(code, r'\n      , std::addressof\(port\)\n')
        code = self.code(probe(optional='sub'))
        self.assertIn('static_cast<Port*>(std::addressof(sub))', code)

    def test_address_of_and_nullptr_are_accepted_for_pointers(self):
        self.assertRegex(self.code(probe(optional='&port')), r'\n      , &port\n')
        self.assertRegex(self.code(probe(optional='nullptr')), r'\n      , static_cast<Port\*>\(nullptr\)\n|\n      , nullptr\n')

    def test_an_earlier_instance_binds_like_a_registration(self):
        code = self.code(led(), {'module': 'Cmd', 'id': 'cmd', 'args': [{'led': 'led'}, {'backup': '&led'}]})
        self.assertRegex(code, r'static Cmd cmd\(\n(#line.*\n)?      led\n(#line.*\n)?      , &led\n')

    def test_reference_and_initializer_list_values_get_static_storage(self):
        self.module('Store', '#include <initializer_list>\nclass Store { public:\n'
                             '  struct Param { int a; };\n'
                             '  Store(const Param& p = {.a = 1}, std::initializer_list<int> values = {1, 2}) {} };')
        code = self.code({'module': 'Store', 'id': 'store', 'args': [{'p': {'a': '2'}}, {'values': ['3', '4']}]})
        self.assertIn('static const Store::Param& xr_arg_store_p =', code)
        self.assertIn('static std::initializer_list<int> xr_arg_store_values =', code)


class Dependencies(GenerationTestCase):
    def test_unknown_name_lists_candidates_of_the_parameter_type(self):
        message = self.error(probe(port='missing'))
        self.assertIn('xrobot.yaml: probe.args.port: missing is neither an XR_REGISTER name nor an earlier '
                      'instance id; candidates of type Port&: port', message)

    def test_unknown_address_for_a_pointer_is_rejected(self):
        self.assertIn('probe.args.optional: missing is neither', self.error(probe(optional='&missing')))

    def test_self_and_later_instances_are_rejected(self):
        message = self.error({'module': 'Cmd', 'id': 'cmd', 'args': [{'led': 'led'}, {'backup': '&cmd'}]},
                             led())
        self.assertIn('cmd.args.led: led is constructed at or after cmd; instances are constructed in list order',
                      message)
        self.assertIn('cmd.args.backup: cmd is constructed at or after cmd', message)

    def test_a_located_type_without_a_public_base_relation_is_rejected(self):
        message = self.error(probe(port='hidden'))
        self.assertIn('probe.args.port: hidden is a Hidden, which does not convert to Port&', message)
        message = self.error(led(), {'module': 'Cmd', 'id': 'cmd', 'args': [{'led': 'port'}, {'backup': 'nullptr'}]})
        self.assertIn('cmd.args.led: port is a Port, which does not convert to Led&', message)

    def test_a_located_pointer_without_a_public_base_relation_is_rejected(self):
        for value in ('&hidden', 'hidden'):
            with self.subTest(value=value):
                self.assertIn('probe.args.optional: hidden is a Hidden, which does not convert to Port*',
                              self.error(probe(optional=value)))

    def test_types_the_index_cannot_locate_are_left_to_the_compiler(self):
        self.code(led(gpio='uart'))

    def test_an_instance_with_errors_is_not_offered_to_later_instances(self):
        message = self.error(led(gpio='missing'), {'module': 'Cmd', 'id': 'cmd', 'args': [{'led': 'led'}, {'backup': 'nullptr'}]})
        self.assertIn('led.args.gpio: missing is neither', message)
        self.assertIn('cmd.args.led: led has errors of its own', message)

    def test_unfilled_dependencies_are_reported(self):
        self.assertIn('probe.args.port is not filled in', self.error(probe(port=None)))


class Names(GenerationTestCase):
    def test_instance_id_equal_to_a_module_class_name_is_rejected(self):
        for identity in ('Led', 'Port', 'SubPort'):
            with self.subTest(identity=identity):
                self.assertIn('instance id %s is also a class name in the loaded Modules' % identity,
                              self.error(led(identity)))

    def test_instance_id_equal_to_a_registration_is_rejected(self):
        self.assertIn('instance id port is also an XR_REGISTER name', self.error(led('port')))

    def test_registration_named_like_a_module_class_is_rejected(self):
        self.entry('#include "xrobot_main.hpp"\nint main() { XR_REGISTER(Port, Port); XROBOT_MAIN(); }\n')
        self.assertIn('XR_REGISTER name Port is also a Module class name', self.error())

    def test_unknown_and_ambiguous_module_names(self):
        self.assertIn('Module not found: Missing', self.error({'module': 'Missing', 'id': 'm'}))
        self.module('Led', 'class Led { public: Led() {} };', owner='other')
        self.assertIn('Ambiguous Module Led; specify', self.error({'module': 'Led', 'id': 'm'}))

    def test_non_standalone_modules_cannot_be_instantiated(self):
        self.module('Base', 'class Base { public: Base() {} };', manifest='/* === MODULE MANIFEST V2 ===\n'
                    'module_description: library\nstandalone: false\n=== END MANIFEST === */\n')
        self.assertIn('team/Base is a non-standalone library, not an instance', self.error({'module': 'Base', 'id': 'b'}))

    def test_unmatched_argument_names_list_the_constructors(self):
        message = self.error({'module': 'Probe', 'id': 'p', 'args': [{'port': 'port'}, {'opt': 'nullptr'}]})
        self.assertIn('named arguments (port, opt) do not match any constructor of Probe; expected one of: '
                      '(port, optional, count, name)', message)


class Diagnostics(GenerationTestCase):
    def test_errors_of_every_instance_are_collected_with_config_and_path_prefixes(self):
        message = self.error(probe('a', port='missing'), led('b', gpio=None), led('Led'))
        lines = message.splitlines()
        self.assertEqual(len(lines), 3, lines)
        self.assertTrue(lines[0].startswith('User/xrobot.yaml: a.args.port: missing is neither'), lines[0])
        self.assertTrue(lines[1].startswith('User/xrobot.yaml: b.args.gpio is not filled in'), lines[1])
        self.assertTrue(lines[2].startswith('User/xrobot.yaml: Led: instance id Led is also a class name'), lines[2])

    def test_structural_errors_name_the_config_file(self):
        with self.assertRaisesRegex(ConfigError, 'User/xrobot.yaml: modules\\[0\\]: unknown key'):
            self.generate({'modules': [{'module': 'Led', 'id': 'led', 'name': 'x'}]})

    def test_a_failed_generation_leaves_the_previous_header(self):
        self.code(led())
        header = self.root / 'User/xrobot_main.hpp'
        before = header.read_bytes()
        self.error(probe(port='missing'))
        self.assertEqual(header.read_bytes(), before)

    def test_regenerating_identical_output_keeps_bytes_and_marks_the_header_fresh(self):
        self.code(led())
        header = self.root / 'User/xrobot_main.hpp'
        before = header.read_bytes()
        past = header.stat().st_mtime - 100
        os.utime(header, (past, past))
        generate(self.project)
        self.assertEqual(header.read_bytes(), before)
        self.assertGreater(header.stat().st_mtime, past)
        self.assertEqual(self.project.header_state()['status'], 'fresh')

    @unittest.skipIf(os.name == 'nt', 'POSIX permission bits')
    def test_regeneration_keeps_the_header_permission_bits(self):
        self.code(led())
        header = self.root / 'User/xrobot_main.hpp'
        header.chmod(0o640)
        self.code(led(), probe())
        self.assertEqual(stat.S_IMODE(header.stat().st_mode), 0o640)

    @unittest.skipIf(os.name == 'nt', 'POSIX permission bits')
    def test_a_new_header_follows_the_umask(self):
        umask = os.umask(0o022)
        try:
            self.code(led())
        finally:
            os.umask(umask)
        self.assertEqual(stat.S_IMODE((self.root / 'User/xrobot_main.hpp').stat().st_mode), 0o644)


class Selection(GenerationTestCase):
    def test_generate_uses_the_selected_product_by_default(self):
        self.config({'modules': [led('default_led')]})
        self.config({'modules': [led('alt_led')]}, name='alt.yaml')
        self.assertIn('default_led', generate(self.project))
        self.assertIn('alt_led', generate(self.project, self.root / 'User/alt.yaml'))
        self.assertIn('alt_led', generate(self.project))

    def test_a_missing_config_is_reported(self):
        with self.assertRaisesRegex(ConfigError, 'User/missing.yaml does not exist'):
            generate(self.project, self.root / 'User/missing.yaml')

    def test_validate_all_checks_every_config_and_collects_errors(self):
        self.config({'modules': [led()]})
        self.config({'modules': [probe(port='missing')]}, name='products/a.yaml')
        self.config({'modules': [led('Led')]}, name='products/b.yaml')
        with self.assertRaises(ConfigError) as context:
            validate_all(self.project)
        message = str(context.exception)
        self.assertIn('User/products/a.yaml: probe.args.port: missing is neither', message)
        self.assertIn('User/products/b.yaml: Led: instance id Led is also a class name', message)
        self.assertNotIn('User/xrobot.yaml', message)
        (self.root / 'User/products/a.yaml').unlink()
        (self.root / 'User/products/b.yaml').unlink()
        self.assertEqual(validate_all(self.project), 1)
        self.assertFalse((self.root / 'User/xrobot_main.hpp').exists())


class Monitors(BspTestCase):
    def setUp(self):
        super().setUp()
        self.entry(MAIN)

    def monitored(self, body, name='M'):
        self.module(name, body)
        code = self.generate({'modules': [{'module': name, 'id': 'm'}]})
        return 'm.OnMonitor();' in code

    def test_a_public_monitor_is_called(self):
        self.assertTrue(self.monitored('class M { public: M() {} void OnMonitor() {} };'))

    def test_a_private_or_missing_monitor_is_not_called(self):
        self.assertFalse(self.monitored('class M { public: M() {} private: void OnMonitor() {} };'))
        self.assertFalse(self.monitored('class M { public: M() {} };'))

    def test_a_monitor_inherited_from_a_module_base_is_called(self):
        self.assertTrue(self.monitored('struct Core { void OnMonitor() {} };\n'
                                       'class M : public Core { public: M() {} };'))
        self.assertFalse(self.monitored('struct Core { void OnMonitor() {} };\n'
                                        'class M : Core { public: M() {} };'))

    def test_using_base_monitor_counts(self):
        self.assertTrue(self.monitored('class Core { protected: void OnMonitor() {} };\n'
                                       'class M : public Core { public: M() {} using Core::OnMonitor; };'))

    def test_libxr_bases_provide_no_monitor(self):
        self.assertFalse(self.monitored('namespace LibXR { class Application {}; }\n'
                                        'class M : public LibXR::Application { public: M() {} };'))

    def test_an_unlocatable_public_base_is_an_error(self):
        self.module('M', 'class M : public Vendor::Base { public: M() {} };')
        with self.assertRaisesRegex(ConfigError, 'm: cannot tell whether a public base class provides OnMonitor'):
            self.generate({'modules': [{'module': 'M', 'id': 'm'}]})

    def test_a_conditional_monitor_is_an_error(self):
        self.module('M', 'class M { public: M() {}\n#if FEATURE\n  void OnMonitor() {}\n#endif\n};')
        with self.assertRaisesRegex(ValueError, 'M declares OnMonitor under #if'):
            self.generate({'modules': [{'module': 'M', 'id': 'm'}]})


class Templates(BspTestCase):
    def setUp(self):
        super().setUp()
        self.entry(MAIN)
        self.module('Buf', 'template <typename T, unsigned N = 4>\nclass Buf { public: explicit Buf(T init = T{}) {} };')
        self.module('Def', 'template <typename T = int>\nclass Def { public: Def() {} };')

    def test_template_arguments_and_defaults(self):
        code = self.generate({'modules': [
            {'module': 'Buf', 'id': 'a', 'template_args': ['float'], 'args': [{'init': '1.0F'}]},
            {'module': 'Buf', 'id': 'b', 'template_args': ['int', '8'], 'args': [{'init': '2'}]},
            {'module': 'Def', 'id': 'c'}]})
        self.assertIn('static Buf<float> a(', code)
        self.assertIn('xrobot_generated::Implicit<float>(1.0F)', code)
        self.assertIn('static Buf<int, 8> b(', code)
        self.assertIn('static Def<> c;', code)

    def test_instance_id_equal_to_a_class_template_name_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'instance id Buf is also a class name in the loaded Modules'):
            self.generate({'modules': [{'module': 'Def', 'id': 'Buf'}]})

    def test_missing_and_extra_template_arguments_are_errors(self):
        with self.assertRaisesRegex(ValueError, 'Template argument Buf.T must be specified'):
            self.generate({'modules': [{'module': 'Buf', 'id': 'a'}]})
        with self.assertRaisesRegex(ValueError, 'Too many template arguments for Def'):
            self.generate({'modules': [{'module': 'Def', 'id': 'a', 'template_args': ['int', 'int']}]})


class Overloads(BspTestCase):
    def setUp(self):
        super().setUp()
        self.entry(MAIN)

    def test_parameter_names_select_the_constructor(self):
        self.module('Foo', 'class Foo { public: Foo(int count = 0) {} Foo(float gain = 0.0f) {} };')
        code = self.generate({'modules': [{'module': 'Foo', 'id': 'f', 'args': [{'gain': '1'}]}]})
        self.assertIn('xrobot_generated::Implicit<float>(1)', code)

    def test_same_names_are_told_apart_only_by_explicit_types(self):
        self.module('Foo', 'class Foo { public: Foo(int value = 0) {} Foo(float value = 0.0f) {} };')
        self.assertIn('Implicit<float>(1.0f)', self.generate({'modules': [
            {'module': 'Foo', 'id': 'f', 'args': [{'value': '1.0f'}]}]}))
        self.assertIn('Implicit<int>(static_cast<int>(Read()))', self.generate({'modules': [
            {'module': 'Foo', 'id': 'f', 'args': [{'value': 'static_cast<int>(Read())'}]}]}))
        with self.assertRaisesRegex(ValueError, 'Foo: constructor is ambiguous'):
            self.generate({'modules': [{'module': 'Foo', 'id': 'f', 'args': [{'value': 'Read()'}]}]})

    def test_a_dependency_after_a_defaulted_parameter_is_not_a_supported_constructor(self):
        self.module('Foo', 'class Foo { public: Foo(int count = 10, Port& port) {} };')
        with self.assertRaisesRegex(ValueError, 'no compliant constructor.*dependency without a default appears after'):
            self.generate({'modules': [{'module': 'Foo', 'id': 'f'}]})


class CompileCheckProbe(BspTestCase):
    def test_probe_uses_null_placeholders_and_is_never_a_program(self):
        self.module('Foo', '''#include <cstdlib>
struct Uart { Uart() = delete; };
template <typename T>
class Foo { public:
  struct Param { int period = 10; T scale{}; };
  static Param Make() { return {}; }
  Foo(Uart& uart, Uart* optional, Param p = {}, Param q = Make()) { std::abort(); }
  void OnMonitor() { std::abort(); }
};''')
        out = self.tmp / 'probe' / 'check.cpp'
        code = generate_compile_check('team/Foo', load_modules(self.project), out, ['float'])
        self.assertEqual(out.read_text(encoding='utf-8'), code)
        self.assertIn('void XRobotCompileCheck() {', code)
        self.assertIn('static void* xr_ci_null = static_cast<void*>(nullptr);', code)
        self.assertIn('static Foo<float> module_0(', code)
        self.assertIn('*static_cast<std::remove_reference_t<Uart&>*>(xr_ci_null)', code)
        self.assertIn('.period = 10\n, .scale = {}', code)
        self.assertIn('static_cast<Foo<float>::Param>(Foo<float>::Make())', code)
        self.assertIn('module_0.OnMonitor();', code)
        for absent in ('int main(', 'for (;;)', '#pragma once', '// xrobot:', 'XROBOT_MAIN'):
            self.assertNotIn(absent, code)

    def test_library_probe_includes_the_header_and_errors_leave_the_output_alone(self):
        self.module('Lib', 'class Lib { public: Lib() {} };', manifest='/* === MODULE MANIFEST V2 ===\n'
                    'standalone: false\n=== END MANIFEST === */\n')
        self.module('Foo', 'class Foo { static int Secret(); public: Foo(int count = Secret()) {} };')
        library = self.tmp / 'library.cpp'
        self.assertEqual(generate_compile_check('team/Lib', load_modules(self.project), library),
                         '#include "Lib.hpp"\n')
        self.assertEqual(library.read_text(encoding='utf-8'), '#include "Lib.hpp"\n')
        out = self.write(self.tmp / 'check.cpp', 'original')
        with self.assertRaisesRegex(ValueError, 'non-public member Foo::Secret'):
            generate_compile_check('team/Foo', load_modules(self.project), out)
        self.assertEqual(out.read_text(encoding='utf-8'), 'original')


@requires_cxx
class GeneratedCpp(CxxMixin, BspTestCase):
    """The generated header compiles, binds, constructs and monitors as specified."""

    def setUp(self):
        super().setUp()
        self.entry(MAIN)

    def test_static_config_lifetime_nested_values_and_pointer_binding(self):
        self.module('Foo', '''#include <cassert>
#include <initializer_list>
struct Left { long padding[3]{}; };
struct Right { int value = 17; };
struct Device : Left, Right {};
class Foo { public:
  struct Param { struct Gain { float kp, ki; }; Gain gain; int number; };
  Foo(Right& right, Right* optional, const Param& p = Param{}, std::initializer_list<int> values = {7, 8})
      : p_(p), values_(values) { assert(&right == optional); assert(right.value == 17); }
  void OnMonitor() const noexcept {
    assert(p_.gain.kp == 1 && p_.gain.ki == 2); assert(p_.number == 3); assert(*values_.begin() == 7);
  }
 private:
  const Param& p_; std::initializer_list<int> values_;
};''')
        self.entry('#include "Foo.hpp"\n#include "xrobot_main.hpp"\n'
                   'int main() { static Device device; XR_REGISTER(device, Device); XROBOT_MAIN(); }\n')
        code = self.generate({'modules': [{'module': 'Foo', 'id': 'f', 'args': [
            {'right': 'device'}, {'optional': 'device'},
            {'p': {'gain': {'kp': '1.0f', 'ki': '2.0f'}, 'number': '3'}}, {'values': ['7', '8']}]}]})
        self.assertIn('static const Foo::Param& xr_arg_f_p', code)
        self.assertIn('static std::initializer_list<int> xr_arg_f_values', code)
        self.compile()

    def test_base_adjustment_and_cv_pointer_storage(self):
        self.stub('thread.hpp', '''#pragma once
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
  Probe(Right& right, int*& pointer, const int& configuration, volatile int& flags, const int* const& fixed) {
    assert(static_cast<const void*>(&right) == expected_right);
    assert(&pointer == expected_storage); assert(configuration == 4);
    assert(*fixed == 4); assert(flags == 5); flags = 6;
    pointer = &alternate; ++constructors;
  }
  ~Probe() { std::abort(); }
  void OnMonitor() { ++monitors; }
};''')
        self.entry('''#include "Probe.hpp"
#include "xrobot_main.hpp"
int main() {
  Device device; int value = 3; int* pointer = &value;
  const int configuration = 4; volatile int flags = 5;
  const int* const fixed = &configuration;
  expected_storage = &pointer;
  expected_right = static_cast<Right*>(&device);
  assert(expected_right != static_cast<void*>(&device));
  XR_REGISTER(device, Device);
  XR_REGISTER(pointer, int*);
  XR_REGISTER(configuration, const int);
  XR_REGISTER(flags, volatile int);
  XR_REGISTER(fixed, const int* const);
  XROBOT_MAIN();
}
''')
        self.generate({'modules': [{'module': 'Probe', 'id': 'probe0', 'args': [
            {'right': 'device'}, {'pointer': 'pointer'}, {'configuration': 'configuration'},
            {'flags': 'flags'}, {'fixed': 'fixed'}]}]})
        sanitizer = [] if 'clang' in Path(CXX).name else ['-fsanitize=undefined', '-fno-sanitize-recover=all']
        self.compile(extra=sanitizer)

    def test_named_overload_is_not_reselected_by_literal_type(self):
        self.module('Foo', '''#include <cassert>
class Foo { public:
  Foo(int count = 0) { (void)count; assert(false && "wrong constructor"); }
  Foo(float gain = 0.0f) { assert(gain == 1.0f); }
};''')
        self.generate({'modules': [{'module': 'Foo', 'id': 'f', 'args': [{'gain': '1'}]}]})
        self.compile()

    def test_factory_prvalue_keeps_guaranteed_copy_elision(self):
        self.module('Foo', '''struct Config {
  Config() = default;
  Config(const Config&) = delete;
  Config(Config&&) = delete;
};
class Foo { public:
  static Config DefaultConfig() { return Config{}; }
  Foo(Config config = DefaultConfig()) { (void)config; }
  Foo(int count = 0) { (void)count; }
};''')
        self.generate({'modules': [{'module': 'Foo', 'id': 'f', 'args': [{'config': 'Foo::DefaultConfig()'}]}]})
        self.compile()

    def test_explicit_only_conversion_is_rejected_by_the_compiler(self):
        self.module('Foo', '''struct Config { explicit Config(int) {} };
class Foo { public:
  Foo(Config config = Config{0}) { (void)config; }
  Foo(int count = 0) { (void)count; }
};''')
        self.generate({'modules': [{'module': 'Foo', 'id': 'f', 'args': [{'config': '1'}]}]})
        self.assertIn('f.args.config requires an implicit conversion to Config', self.compile(expected=False))

    def test_downcast_of_an_unlocatable_type_is_rejected_by_the_compiler(self):
        self.stub('libxr.hpp', '#pragma once\nnamespace LibXR { struct CAN {}; struct FDCAN : CAN {}; }\n')
        self.module('Motor', 'class Motor { public: explicit Motor(LibXR::FDCAN& can) { (void)can; } };')
        self.entry('#include "xrobot_main.hpp"\n'
                   'int main() { static LibXR::FDCAN fdcan1; LibXR::CAN& can1 = fdcan1;\n'
                   '  XR_REGISTER(can1, LibXR::CAN); XROBOT_MAIN(); }\n')
        self.generate({'modules': [{'module': 'Motor', 'id': 'motor', 'args': [{'can': 'can1'}]}]})
        self.assertIn('motor.args.can requires an implicit conversion to LibXR::FDCAN&', self.compile(expected=False))

    def test_a_reference_alias_registration_binds_the_base(self):
        self.stub('libxr.hpp', '#pragma once\nnamespace LibXR { struct CAN { int id = 5; }; struct FDCAN : CAN {}; }\n')
        self.module('Motor', '#include <cassert>\nclass Motor { public: explicit Motor(LibXR::CAN& can) '
                             '{ assert(can.id == 5); } };')
        self.entry('#include "xrobot_main.hpp"\n'
                   'int main() { static LibXR::FDCAN fdcan1; LibXR::CAN& can1 = fdcan1;\n'
                   '  XR_REGISTER(can1, LibXR::CAN); XROBOT_MAIN(); }\n')
        self.generate({'modules': [{'module': 'Motor', 'id': 'motor', 'args': [{'can': 'can1'}]}]})
        self.compile()

    def test_constant_narrowing_keeps_the_compiler_warning(self):
        self.module('Foo', '#include <cstdint>\nclass Foo { public: explicit Foo(std::uint8_t id = 1) { (void)id; } };')
        self.generate({'modules': [{'module': 'Foo', 'id': 'f', 'args': [{'id': '300'}]}]})
        self.assertIn('44', self.compile(expected=False))

    def test_monitor_order_and_accepted_signatures(self):
        self.stub('thread.hpp', '''#pragma once
#include <cstdlib>
#include <cassert>
inline int stage = 0, extra = 0;
namespace LibXR { struct Thread { static void Sleep(unsigned) { assert(stage == 4); assert(extra == 2); std::_Exit(0); } }; }
''')
        self.module('A', '#include "thread.hpp"\nclass A { public: A() { assert(stage++ == 0); } '
                         'void OnMonitor() { assert(stage++ == 2); } };')
        self.module('B', '#include "thread.hpp"\n#include "A.hpp"\nclass B { public: explicit B(A& a) '
                         '{ (void)a; assert(stage++ == 1); } void OnMonitor() const noexcept { assert(stage++ == 3); } };')
        variants = [('NoMonitor', ''), ('Private', 'private: void OnMonitor() { std::abort(); }'),
                    ('Defaulted', 'void OnMonitor(int = 0) { ++extra; }'),
                    ('Overloaded', 'void OnMonitor() { ++extra; } void OnMonitor(int) { std::abort(); }')]
        for name, monitor in variants:
            self.module(name, '#include "thread.hpp"\nclass %s { public: %s() {} %s };' % (name, name, monitor))
        entries = [{'module': 'A', 'id': 'a'}, {'module': 'B', 'id': 'b', 'args': [{'a': 'a'}]}]
        entries += [{'module': name, 'id': name.lower() + '0'} for name, _ in variants]
        code = self.generate({'modules': entries})
        self.assertNotIn('nomonitor0.OnMonitor', code)
        self.assertNotIn('private0.OnMonitor', code)
        self.compile()

    def test_monitor_with_a_non_void_result_fails_to_compile(self):
        self.module('Wrong', 'class Wrong { public: Wrong() {} int OnMonitor() { return 1; } };')
        self.generate({'modules': [{'module': 'Wrong', 'id': 'wrong'}]})
        self.assertIn('wrong.OnMonitor() must return void', self.compile(expected=False))

    def test_inherited_monitor_is_called(self):
        self.stub('thread.hpp', '#pragma once\n#include <cstdlib>\ninline int calls = 0;\n'
                                'namespace LibXR { struct Thread { static void Sleep(unsigned) '
                                '{ std::_Exit(calls == 1 ? 0 : 3); } }; }\n')
        self.module('Derived', '#include "thread.hpp"\nstruct DerivedCore { void OnMonitor() { ++calls; } };\n'
                               'class Derived : public DerivedCore { public: Derived() {} };')
        self.assertIn('derived.OnMonitor();', self.generate({'modules': [{'module': 'Derived', 'id': 'derived'}]}))
        self.compile()

    def test_local_caller_defined_registered_types(self):
        self.module('Probe', '#include <cassert>\ntemplate <class T> class Probe { public: explicit Probe(T& value) '
                             '{ assert(value.number == 7); } };')
        entry = {'module': 'Probe', 'id': 'p', 'template_args': ['std::remove_reference_t<decltype(dev)>'],
                 'args': [{'value': 'dev'}]}
        mains = {
            'local struct': 'int main() { struct Local { int number = 7; }; static Local dev; '
                            'XR_REGISTER(dev, Local); XROBOT_MAIN(); }',
            'anonymous typedef': 'int main() { typedef struct { int number; } Local; static Local dev{7}; '
                                 'XR_REGISTER(dev, Local); XROBOT_MAIN(); }',
            'declared after the header': 'struct View { int number = 7; };\n'
                                         'int main() { static View dev; XR_REGISTER(dev, View); XROBOT_MAIN(); }',
            'local alias of a base': 'struct Port { int number = 7; }; struct Device : Port { int number = 1; };\n'
                                     'int main() { using LocalView = Port; static Device dev; '
                                     'XR_REGISTER(dev, LocalView); XROBOT_MAIN(); }',
        }
        for label, body in mains.items():
            with self.subTest(label):
                self.entry('#include "xrobot_main.hpp"\n' + body + '\n')
                self.generate({'modules': [entry]})
                self.compile()

    def test_caller_array_bounds_and_non_type_template_arguments(self):
        self.module('Probe', '#include <cassert>\ntemplate <class T> class Probe { public: explicit Probe(T& value) '
                             '{ assert(value.size() == 2); } };')
        entry = {'module': 'Probe', 'id': 'p', 'template_args': ['std::remove_reference_t<decltype(values)>'],
                 'args': [{'value': 'values'}]}
        mains = [
            'int main() { enum { Count = 2 }; static std::array<int, Count> values{}; '
            'XR_REGISTER(values, std::array<int, Count>); XROBOT_MAIN(); }',
            'int main() { constexpr unsigned Count{2}; static std::array<int, Count> values{}; '
            'XR_REGISTER(values, std::array<int, Count>); XROBOT_MAIN(); }',
            'template <unsigned Count> void Launch() { static std::array<int, Count> values{}; '
            'XR_REGISTER(values, std::array<int, Count>); XROBOT_MAIN(); }\nint main() { Launch<2>(); }',
        ]
        for body in mains:
            with self.subTest(body=body):
                self.entry('#include <array>\n#include "xrobot_main.hpp"\n' + body + '\n')
                self.generate({'modules': [entry]})
                self.compile()

    def test_generated_template_parameters_do_not_shadow_user_names(self):
        self.module('Probe', 'template <class T> class Probe { public: explicit Probe(T& value) { value.number = 7; } };')
        self.entry('#include "xrobot_main.hpp"\nint main() { struct Local { int number = 1; }; static Local XrViewType0; '
                   'XR_REGISTER(XrViewType0, Local); XROBOT_MAIN(); }\n')
        self.generate({'modules': [{'module': 'Probe', 'id': 'XrViewType1',
                                    'template_args': ['std::remove_reference_t<decltype(XrViewType0)>'],
                                    'args': [{'value': 'XrViewType0'}]}]})
        self.compile()

    def test_array_and_function_pointer_registrations(self):
        self.module('Probe', '''#include <cassert>
using Array = int[2];
using Callback = int (*)(int);
class Probe { public:
  Probe(Array& values, Callback& fn) { assert(values[0] == 3 && values[1] == 5); assert(fn(4) == 8); values[1] = 9; }
};''')
        self.entry('#include "xrobot_main.hpp"\nint Twice(int x) { return x * 2; }\n'
                   'int main() { static Array values = {3, 5}; static Callback fn = Twice;\n'
                   '  XR_REGISTER(values, Array); XR_REGISTER(fn, Callback); XROBOT_MAIN(); }\n')
        code = self.generate({'modules': [{'module': 'Probe', 'id': 'p', 'args': [{'values': 'values'}, {'fn': 'fn'}]}]})
        self.assertIn('    Array& values,\n    Callback& fn)', code)
        self.compile()

    def test_caller_function_pointer_typedef(self):
        self.module('Probe', '#include <cassert>\ntemplate <class T> class Probe { public: explicit Probe(T& fn) '
                             '{ assert(fn(3) == 6); } };')
        self.entry('#include "xrobot_main.hpp"\nint Twice(int x) { return x * 2; }\n'
                   'int main() { typedef int (*LocalCallback)(int); static LocalCallback fn = Twice; '
                   'XR_REGISTER(fn, LocalCallback); XROBOT_MAIN(); }\n')
        self.generate({'modules': [{'module': 'Probe', 'id': 'p',
                                    'template_args': ['std::remove_reference_t<decltype(fn)>'], 'args': [{'fn': 'fn'}]}]})
        self.compile()

    def test_pointer_covariance_through_a_reference_is_rejected(self):
        self.module('Probe', 'struct Base {}; struct Derived : Base {};\n'
                             'class Probe { public: explicit Probe(Base*& pointer) { (void)pointer; } };')
        self.entry('#include "Probe.hpp"\n#include "xrobot_main.hpp"\n'
                   'int main() { static Derived d; static Derived* ptr = &d; XR_REGISTER(ptr, Derived*); XROBOT_MAIN(); }\n')
        self.generate({'modules': [{'module': 'Probe', 'id': 'p', 'args': [{'pointer': 'ptr'}]}]})
        self.compile(expected=False)

    def test_const_object_cannot_bind_a_mutable_reference(self):
        self.module('Probe', 'class Probe { public: explicit Probe(int& value) { (void)value; } };')
        for registration in ('int', 'const int'):
            with self.subTest(registration=registration):
                self.entry('#include "xrobot_main.hpp"\nint main() { static const int value = 1; '
                           'XR_REGISTER(value, %s); XROBOT_MAIN(); }\n' % registration)
                self.generate({'modules': [{'module': 'Probe', 'id': 'p', 'args': [{'value': 'value'}]}]})
                self.compile(expected=False)

    def test_one_object_binds_distinct_and_virtual_bases(self):
        self.module('Probe', '''#include <cassert>
struct Port { virtual ~Port() = default; virtual int Read() { return 1; } };
struct Left : virtual Port {};
struct Right { int value = 7; };
struct Device : Left, Right { int Read() override { return 11; } };
class Probe { public: Probe(Port& p, Right& r) { assert(p.Read() == 11); assert(r.value == 7); } };''')
        self.entry('#include "Probe.hpp"\n#include "xrobot_main.hpp"\n'
                   'int main() { static Device dev; XR_REGISTER(dev, Device); XROBOT_MAIN(); }\n')
        self.generate({'modules': [{'module': 'Probe', 'id': 'p', 'args': [{'p': 'dev'}, {'r': 'dev'}]}]})
        self.compile()

    def test_an_ambiguous_base_is_rejected_by_the_compiler(self):
        self.module('Probe', 'struct Port {}; struct A : Port {}; struct B : Port {}; struct Twice : A, B {};\n'
                             'class Probe { public: explicit Probe(Port& p) { (void)p; } };')
        self.entry('#include "Probe.hpp"\n#include "xrobot_main.hpp"\n'
                   'int main() { static Twice dev; XR_REGISTER(dev, Twice); XROBOT_MAIN(); }\n')
        self.generate({'modules': [{'module': 'Probe', 'id': 'p', 'args': [{'p': 'dev'}]}]})
        self.compile(expected=False)

    def test_a_later_instance_used_inside_an_expression_fails_in_cpp(self):
        self.module('A', 'class A { public: A(int n = 1) { (void)n; } int Get() { return 1; } };')
        self.generate({'modules': [{'module': 'A', 'id': 'a', 'args': [{'n': 'b.Get()'}]},
                                   {'module': 'A', 'id': 'b', 'args': [{'n': '1'}]}]})
        self.compile(expected=False)

    def test_no_instances_and_no_registrations_compile_pedantically(self):
        self.generate({'modules': []})
        self.compile(extra=['-pedantic-errors'])

    def test_a_registration_must_name_an_object_visible_at_the_call(self):
        self.module('A', 'class A { public: explicit A(int& value) { (void)value; } };')
        self.entry('#include "xrobot_main.hpp"\nint main() { XR_REGISTER(v, int); XROBOT_MAIN(); int v = 1; (void)v; }\n')
        self.generate({'modules': [{'module': 'A', 'id': 'a', 'args': [{'value': 'v'}]}]})
        self.compile(expected=False)

    def test_entry_function_can_be_called_directly(self):
        self.module('Probe', 'struct Port {}; class Probe { public: explicit Probe(Port& port) { (void)port; } '
                             'void OnMonitor() {} };')
        self.entry('#include "Probe.hpp"\n#include "xrobot_main.hpp"\n'
                   'static Port port;\nvoid Run() { XR_REGISTER(port, Port); XROBOT_MAIN(); }\n'
                   'int main() { ::XRobotMain(port); }\n')
        self.generate({'modules': [{'module': 'Probe', 'id': 'probe_0', 'args': [{'port': 'port'}]}]})
        self.compile()

    def test_constructor_errors_are_diagnosed_without_a_call(self):
        self.module('Probe', 'class Probe { public: explicit Probe(int value) { (void)value; } };')
        self.entry('#include "xrobot_main.hpp"\nint main() { return 0; }\n// XROBOT_MAIN();\n'
                   'void Run() { XROBOT_MAIN(); }\n')
        self.generate({'modules': [{'module': 'Probe', 'id': 'p', 'args': [{'value': '"wrong"'}]}]})
        self.compile(expected=False)

    def test_header_is_self_contained_and_idempotent(self):
        self.module('Probe', 'class Probe { public: explicit Probe(int value = 3) { (void)value; } };')
        self.generate({'modules': [{'module': 'Probe', 'id': 'p', 'args': [{'value': '3'}]}]})
        self.entry('#include "xrobot_main.hpp"\n#include "xrobot_main.hpp"\nint main() { return 0; }\n'
                   'void Run() { XROBOT_MAIN(); }\n')
        self.compile()

    def test_the_compile_check_probe_compiles_but_is_never_linked(self):
        self.module('Foo', '''#include <cstdlib>
struct Uart { Uart() = delete; };
struct FooParam { int period = 10; float scale = 1.0f; };
class Foo { public:
  Foo(Uart& uart, Uart* optional, FooParam param = {}, int period = 10) {
    (void)uart; (void)optional; (void)param; (void)period; std::abort(); }
  void OnMonitor() { std::abort(); }
};''')
        out = self.tmp / 'check.cpp'
        generate_compile_check('team/Foo', load_modules(self.project), out)
        self.compile(out, execute=False)


@requires_cxx
class LineDirectivesInCpp(CxxMixin, BspTestCase):
    """Compiler diagnostics point into the YAML for values and back into the header elsewhere."""

    def setUp(self):
        super().setUp()
        self.entry(MAIN)

    def test_a_value_of_the_wrong_type_is_reported_at_its_yaml_line(self):
        self.module('Probe', 'class Probe { public: explicit Probe(int value = 1, const char* name = "p") '
                             '{ (void)value; (void)name; } };')
        config = self.config('modules:\n  - module: Probe\n    id: p\n    args:\n'
                             '      - value: 1\n      - name: 42\n')
        generate(self.project, config)
        output = self.compile(expected=False)
        self.assertRegex(output, r'xrobot\.yaml:6:\d*:? ?\s*(error|note)', output)
        self.assertIn('p.args.name requires an implicit conversion to const char*', output)

    def test_an_arithmetic_value_error_is_reported_at_its_yaml_line(self):
        self.module('Probe', 'class Probe { public: explicit Probe(int first = 1, int value = 1) '
                             '{ (void)first; (void)value; } };')
        config = self.config('# product\n\nmodules:\n  - module: Probe\n    id: p\n    args:\n'
                             '      - first: 2\n      - value: "\\"wrong\\""\n')
        generate(self.project, config)
        output = self.compile(expected=False)
        self.assertRegex(output, r'xrobot\.yaml:8:\d+: error', output)

    def test_errors_after_an_instance_are_reported_in_the_header(self):
        self.module('Wrong', 'class Wrong { public: Wrong() {} int OnMonitor() { return 1; } };')
        config = self.config('modules:\n  - module: Wrong\n    id: wrong\n')
        code = generate(self.project, config)
        line = next(i for i, text in enumerate(code.split('\n'), 1) if 'wrong.OnMonitor() must return void' in text)
        output = self.compile(expected=False)
        self.assertRegex(output, r'xrobot_main\.hpp:%d:\d+: error' % line, output)


if __name__ == '__main__':
    unittest.main()
