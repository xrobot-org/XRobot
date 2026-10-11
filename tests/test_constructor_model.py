"""构造参数和值的规则（xrobot.constructor_model）：参数声明、构造函数选择、映射检查和类型转换。
Constructor parameter and value rules (xrobot.constructor_model): declarations, constructor
selection, mapping checks and conversions.
"""

from fixtures import BspTestCase, CxxMixin, TestCase, requires_cxx

from xrobot.constructor_model import (
    argument_text,
    binding_candidates,
    compliant_constructors,
    constructor_for,
    enrich_interface,
    explicit_expression_type,
    initializer_tree,
    is_arithmetic,
    is_dependency,
    parameter,
    qualify,
    template_bindings,
    type_shape,
)
from xrobot.source_syntax import extract_interface
from xrobot.type_index import class_scope_names

MAIN = '#include "xrobot_main.hpp"\nint main() { XROBOT_MAIN(); }\n'
# 最外层的结构体值每个字段一行，行尾带逗号。
# An outermost struct value has one field per line, each followed by a comma.
FIELDS_A_B = "      .a = 1,\n      .b = 3,\n"
FIELDS_X_Y = "      .x = 3,\n      .y = 4,\n"


def interface(source, name="Foo"):
    """头文件文本中一个类的构造接口，含参数形状。
    The constructor interface of a class in a header text, with parameter shapes.
    """
    return enrich_interface(source, extract_interface(source, name))


class MappingTestCase(BspTestCase):
    """带结构体参数模块的 BSP，供映射测试使用。
    A BSP with Modules that take struct parameters, for the mapping tests.
    """

    def setUp(self):
        super().setUp()
        self.entry(MAIN)

    def value(self, module, parameter_name, value, identity="p"):
        """用一个参数值生成头文件并返回其文本，不含 #line 指令（它们不属于值的写法）。
        Generate the header with one argument value and return its text without the #line
        directives, which are not part of how the value is written.
        """
        code = self.generate(
            {"modules": [{"module": module, "id": identity, "args": [{parameter_name: value}]}]}
        )
        return "\n".join(line for line in code.split("\n") if not line.startswith("#line"))

    def rejected(self, module, parameter_name, value, pattern):
        """断言这个参数值被拒绝，报错匹配 pattern。
        Assert that the argument value is rejected with an error matching pattern.
        """
        with self.assertRaisesRegex(ValueError, pattern):
            self.value(module, parameter_name, value)


class Aggregates(MappingTestCase):
    """聚合体参数的映射：字段、顺序和嵌套。
    Mappings of aggregate parameters: fields, order and nesting.
    """

    def setUp(self):
        super().setUp()
        self.module(
            "P",
            "class P { public:\n  struct Param { int a; int b = 2; };\n"
            "  static constexpr Param kDefault{1, 2};\n  static Param Make() { return {}; }\n"
            "  explicit P(Param param = {.a = 1, .b = 2}) {}\n};",
        )

    def test_a_mapping_lists_every_data_member_in_order(self):
        self.assertIn(FIELDS_A_B, self.value("P", "param", {"a": "1", "b": "3"}))
        self.rejected(
            "P",
            "param",
            {"a": "1"},
            r"p\.args\.param: missing b \(data members of P::Param\); expected: a, b",
        )
        self.rejected("P", "param", {"a": "1", "b": "2", "c": "3"}, r"p\.args\.param: unknown c")
        self.rejected("P", "param", {"b": "3", "a": "1"}, "fields out of declaration order")

    def test_positional_values_are_rejected_for_located_types(self):
        self.rejected(
            "P",
            "param",
            ["1", "2"],
            "positional values are not accepted for P::Param; write a mapping",
        )
        for text in ("{1, 2}", "P::Param{1, 2}", "Param{1,2}"):
            with self.subTest(text=text):
                self.rejected(
                    "P", "param", text, "positional initializer .* is not accepted for P::Param"
                )

    def test_designated_brace_text_is_checked_like_a_mapping(self):
        self.assertIn(FIELDS_A_B, self.value("P", "param", "{.a = 1, .b = 3}"))
        self.rejected("P", "param", "{.a = 1}", r"p\.args\.param: missing b")
        self.rejected("P", "param", "{.b = 1, .a = 2}", "fields out of declaration order")

    def test_type_prefixed_designated_brace_text_is_checked_like_a_mapping(self):
        self.rejected("P", "param", "P::Param{.a = 1}", r"p\.args\.param: missing b")

    def test_empty_braces_constants_and_factory_calls_are_accepted(self):
        for text in ("{}", "P::Param{}", "P::kDefault", "P::Make()"):
            with self.subTest(text=text):
                self.value("P", "param", text)

    def test_nested_and_cross_header_structs_are_checked(self):
        self.module(
            "Shared",
            "namespace common { struct Preview { bool enabled{false}; int port{1}; }; }\n"
            "class Shared { public: Shared() {} };",
        )
        self.module(
            "R",
            '#include "Shared.hpp"\nclass R { public:\n'
            "  struct Config { struct Inner { int x{0}; } inner; common::Preview preview{}; };\n"
            "  explicit R(Config cfg = {}) {} };",
        )
        self.rejected(
            "R",
            "cfg",
            {"inner": {"x": "1"}, "preview": {"enabled": "true"}},
            r"p\.args\.cfg\.preview: missing port",
        )
        self.rejected(
            "R",
            "cfg",
            {"inner": {}, "preview": {"enabled": "true", "port": "2"}},
            r"p\.args\.cfg\.inner: missing x",
        )
        self.rejected(
            "R",
            "cfg",
            {"inner": ["1"], "preview": {"enabled": "true", "port": "2"}},
            r"p\.args\.cfg\.inner: positional values",
        )
        code = self.value(
            "R", "cfg", {"inner": {"x": "1"}, "preview": "{.enabled = true, .port = 2}"}
        )
        self.assertIn(".preview = {.enabled = true, .port = 2}", code)

    def test_a_factory_default_does_not_relax_the_struct_definition(self):
        self.module(
            "Q",
            'class Q { public:\n  struct Config { int kept{1}; const char* topic{"x"}; };\n'
            "  static Config DefaultConfig() { return {}; }\n"
            "  explicit Q(Config cfg = DefaultConfig()) {} };",
        )
        self.rejected(
            "Q", "cfg", {"kept": "2"}, r"p\.args\.cfg: missing topic \(data members of Q::Config\)"
        )
        self.assertIn('.topic = "y"', self.value("Q", "cfg", {"kept": "2", "topic": '"y"'}))

    def test_member_types_are_qualified_in_generated_initializers(self):
        self.module(
            "E",
            "class E { public:\n  enum class Mode { A, B };\n  struct Param { Mode mode; };\n"
            "  explicit E(Param param = {}) {} };",
        )
        self.assertIn(".mode = E::Mode::B", self.value("E", "param", {"mode": "E::Mode::B"}))


class Unmappable(MappingTestCase):
    """无法检查字段的类型拒绝映射。
    Types whose fields cannot be checked reject mappings.
    """

    def test_types_that_cannot_be_checked_reject_mappings(self):
        self.module(
            "U",
            """class U { public:
  struct WithUnion { int a; union { int x; float y; }; };
  struct Conditional { int a;
#if FEATURE
    int b;
#endif
  };
  struct Base { int a; };
  struct Derived : Base {};
  struct Virtual { virtual void f(); int a; };
  U(WithUnion u = {}, Conditional c = {}, Derived d = {}, Virtual v = {}) {}
};""",
        )
        base = [{"u": "{}"}, {"c": "{}"}, {"d": "{}"}, {"v": "{}"}]
        cases = [
            (0, "U::WithUnion contains a union"),
            (1, r"U::Conditional declares fields under #if \(b\)"),
            (2, "U::Derived has base classes or virtual functions and no constructor"),
            (3, "U::Virtual has base classes or virtual functions and no constructor"),
        ]
        self.generate({"modules": [{"module": "U", "id": "u", "args": base}]})
        for index, reason in cases:
            with self.subTest(reason=reason):
                args = [dict(a) for a in base]
                args[index] = {list(base[index])[0]: {"a": "1"}}
                with self.assertRaisesRegex(
                    ValueError, reason + "; write this value as a complete C\\+\\+ expression"
                ):
                    self.generate({"modules": [{"module": "U", "id": "u", "args": args}]})


class UnlocatableTypes(MappingTestCase):
    """索引中找不到的类型：映射按默认初始化器检查。
    Types the index cannot locate: mappings are checked against the default initializer.
    """

    def setUp(self):
        super().setUp()
        self.module(
            "X",
            "#include <external.hpp>\nclass X { public:\n"
            "  X(External::Cfg designated = {.x = 1, .y = 2}, External::Cfg plain = {}) {}\n};",
        )

    def generate_x(self, designated="{}", plain="{}"):
        """用两个参数值生成模块 X 的实例。
        Generate an instance of Module X with the two argument values.
        """
        return self.generate(
            {
                "modules": [
                    {
                        "module": "X",
                        "id": "x",
                        "args": [{"designated": designated}, {"plain": plain}],
                    }
                ]
            }
        )

    def test_a_mapping_must_match_the_designated_default(self):
        self.assertIn(FIELDS_X_Y, self.generate_x(designated={"x": "3", "y": "4"}))
        with self.assertRaisesMessage(
            ValueError,
            "User/xrobot.yaml: x.args.designated: missing y (from the default initializer); "
            "expected: x, y",
        ):
            self.generate_x(designated={"x": "3"})
        with self.assertRaisesMessage(
            ValueError,
            "User/xrobot.yaml: x.args.designated: fields out of declaration order (from the "
            "default initializer); expected: x, y",
        ):
            self.generate_x(designated={"y": "3", "x": "4"})

    def test_without_a_designated_default_a_mapping_is_rejected(self):
        with self.assertRaisesMessage(
            ValueError,
            "User/xrobot.yaml: x.args.plain: cannot verify the fields of External::Cfg in the "
            "loaded Module headers; write this value as a complete C++ expression",
        ):
            self.generate_x(plain={"x": "1"})

    def test_lists_and_brace_text_are_left_to_the_compiler(self):
        self.generate_x(designated=["1", "2"], plain="External::Cfg{1}")
        self.generate_x(plain=["1"])
        self.generate_x(plain="{1, 2}")


class ConstructorMappings(MappingTestCase):
    """有构造函数的类的映射。
    Mappings of classes with constructors.
    """

    def setUp(self):
        super().setUp()
        self.module(
            "S",
            "class S { public:\n  enum class Mode { A, B };\n  struct Runtime {\n"
            "    Runtime() = default;\n"
            "    Runtime(Mode mode, unsigned level, unsigned long long settle = 10U) {}\n"
            "    Runtime(Mode mode, unsigned legacy_div, unsigned level, float legacy_hz = 1.0F) {}\n"
            "  };\n  explicit S(Runtime runtime = {}) {} };",
        )

    def test_names_select_one_constructor_and_values_are_written_as_they_are(self):
        code = self.value(
            "S",
            "runtime",
            {"mode": "S::Mode::A", "legacy_div": "3", "level": "1", "legacy_hz": "100.0F"},
        )
        self.assertIn(
            "  static const S::Runtime xr_p_runtime = S::Runtime(S::Mode::A, 3, 1, 100.0F);\n",
            code,
        )
        self.assertNotIn("Implicit", code)

    def test_values_are_converted_when_several_constructors_take_the_call(self):
        self.module(
            "T",
            "class T { public:\n  enum class Mode { A, B };\n  struct Runtime {\n"
            "    Runtime(Mode mode, unsigned level) {}\n    Runtime(Mode mode, float gain) {}\n"
            "  };\n  explicit T(Runtime runtime) {} };",
        )
        code = self.value("T", "runtime", {"mode": "T::Mode::A", "gain": "1"})
        self.assertIn(
            "T::Runtime(static_cast<T::Mode>(T::Mode::A), xrobot_generated::Implicit<float>(1))",
            " ".join(code.split()),
        )

    def test_the_mapping_must_name_every_parameter_of_one_constructor(self):
        self.rejected(
            "S",
            "runtime",
            {"mode": "S::Mode::A", "level": "1"},
            r"p\.args\.runtime: S::Runtime has constructors; the mapping must name one constructor's "
            r"parameters in order\. Constructors: mode, level, settle \| mode, legacy_div, level, legacy_hz",
        )
        self.rejected(
            "S", "runtime", {"level": "1", "mode": "S::Mode::A", "settle": "1"}, "Constructors:"
        )
        # 只有默认构造函数的类没有可以写出的参数。
        # A class with only a default constructor has no parameters to name.
        self.module(
            "Q",
            "class Q { public:\n  struct Plain { Plain() {} };\n  explicit Q(Plain plain = {}) {} };",
        )
        with self.assertRaisesMessage(
            ValueError,
            "User/xrobot.yaml: p.args.plain: Q::Plain has constructors; the mapping must name one "
            "constructor's parameters in order. Constructors: none",
        ):
            self.value("Q", "plain", {"x": "1"})

    def test_positional_values_are_rejected_for_classes(self):
        self.rejected(
            "S", "runtime", ["S::Mode::A", "1"], "positional values are not accepted for S::Runtime"
        )
        self.rejected("S", "runtime", "S::Runtime{S::Mode::A, 1U}", "positional initializer")

    def test_constructor_expressions_are_accepted(self):
        self.value("S", "runtime", "S::Runtime(S::Mode::A, 1U)")


@requires_cxx
class MappingsInCpp(CxxMixin, MappingTestCase):
    """映射生成的 C++ 能够编译。
    The C++ generated from mappings compiles.
    """

    def test_constructor_mapping_of_a_nested_template_member(self):
        self.module(
            "Sync",
            """#include <string_view>
#include <cstdlib>
struct Layout { static constexpr int width = 4; };
template <typename L>
class Sync {
 public:
  enum class Mode { LATEST, NEAREST };
  struct RuntimeParam {
    RuntimeParam() = default;
    constexpr RuntimeParam(Mode mode, int offset, std::string_view topic, std::string_view raw = {})
        : mode(mode), offset(offset + L::width), topic(topic), raw(raw) {}
    constexpr RuntimeParam(Mode mode, unsigned div, int offset, float hz = 1.0F)
        : mode(mode), offset(offset + static_cast<int>(div)) { (void)hz; }
    Mode mode = Mode::LATEST;
    int offset = 0;
    std::string_view topic{};
    std::string_view raw{};
  };
  explicit Sync(const RuntimeParam& param = {}) {
    if (param.mode != Mode::NEAREST || param.offset != 7 || param.topic != "cmd" || !param.raw.empty()) std::abort();
  }
};""",
        )
        self.generate(
            {
                "modules": [
                    {
                        "module": "Sync",
                        "id": "sync",
                        "template_args": ["Layout"],
                        "args": [
                            {
                                "param": {
                                    "mode": "Sync<Layout>::Mode::NEAREST",
                                    "offset": "3",
                                    "topic": '"cmd"',
                                    "raw": "{}",
                                }
                            }
                        ],
                    }
                ]
            }
        )
        self.compile()

    def test_complete_aggregate_mappings_compile_with_member_defaults_replaced(self):
        self.module(
            "A",
            """#include <cstdlib>
struct Gains { float kp = 0; float ki = 0; };
struct Param { Gains gains; int count = 1; const char* name = "a"; };
class A { public:
  explicit A(Param param = {}) {
    if (param.gains.kp != 2.0f || param.gains.ki != 0.5f || param.count != 3 || param.name[0] != 'x') std::abort();
  }
};""",
        )
        self.value(
            "A", "param", {"gains": {"kp": "2.0f", "ki": "0.5f"}, "count": "3", "name": '"x"'}
        )
        self.compile()


class ConstructorModel(TestCase):
    """参数声明、名字限定、构造函数选择和类型转换规则。
    Parameter declarations, name qualification, constructor selection and conversion rules.
    """

    def test_parameters_need_explicit_names(self):
        p = parameter("const std::array<float, 2>& gains = {1.0f, 0.0f}")
        self.assertEqual(
            (p["name"], p["type"], p["default"]),
            ("gains", "const std::array<float, 2>&", "{1.0f, 0.0f}"),
        )
        for declaration in ("LibXR::UART&", "int (*fn)(int)", "int values[3]", "int x ="):
            with self.subTest(declaration=declaration), self.assertRaises(ValueError):
                parameter(declaration)

    def test_initializer_tree_expands_only_explicit_braces(self):
        self.assertEqual(initializer_tree("{}"), [])
        self.assertEqual(initializer_tree("Foo::Factory(1, 2)"), "Foo::Factory(1, 2)")
        self.assertEqual(
            initializer_tree("Param{.gain={1,2},.flag=true}", "const Param&"),
            {"gain": ["1", "2"], "flag": "true"},
        )
        self.assertEqual(initializer_tree("Derived{7}", "Base"), "Derived{7}")
        self.assertEqual(initializer_tree("[]{ return 7; }"), "[]{ return 7; }")
        self.assertEqual(
            initializer_tree("{.inner=Vendor::Options{7}}"), {"inner": "Vendor::Options{7}"}
        )
        self.assertEqual(initializer_tree("{.a = 1, 2}"), "{.a = 1, 2}")
        with self.assertRaisesMessage(ValueError, "Duplicate initializer field: a"):
            initializer_tree("{.a = 1, .a = 2}")

    def test_public_names_are_qualified_and_private_ones_rejected(self):
        model = interface(
            "class Foo { private: static int Secret(); public: enum class Mode {A}; "
            "Foo(Mode mode = Mode::A) {} };"
        )
        self.assertEqual(qualify("Mode::A", model, "Foo"), "Foo::Mode::A")
        self.assertEqual(qualify("Vendor::Secret()", model, "Foo"), "Vendor::Secret()")
        self.assertEqual(qualify("{.Secret=4}", model, "Foo"), "{.Secret=4}")
        for text in ("Foo::Secret()", "Secret()"):
            with (
                self.subTest(text=text),
                self.assertRaisesMessage(
                    ValueError, "Expression uses non-public member Foo::Secret"
                ),
            ):
                qualify(text, model, "Foo")

    def test_visible_names_come_from_the_class_definition(self):
        source = (
            "struct Helper { enum Level { LOW, HIGH }; };\n"
            "class Bar { public:\n  enum Speed { SLOW, FAST };\n  union Raw { int i; float f; };\n"
            "  static constexpr int kTable[2] = {1, 2};\n  typedef enum { ON, OFF } Power;\n"
            "  template <class T> struct Box { T value; };\n  friend struct Tester;\n"
            " private:\n  static int Secret();\n  int field;\n"
            " public:\n  explicit Bar(Speed speed = SLOW) {} };"
        )
        names, aliases = class_scope_names(source, "Bar")
        self.assertEqual(
            names,
            {
                "Speed": "public",
                "SLOW": "public",
                "FAST": "public",
                "Raw": "public",
                "kTable": "public",
                "ON": "public",
                "OFF": "public",
                "Power": "public",
                "Box": "public",
                "Secret": "private",
            },
        )
        self.assertEqual(aliases, {})

    def test_aliases_are_followed_only_on_request(self):
        model = interface(
            "class Foo { public: typedef struct { int n; } Param;\n"
            "  using Options = Param; Foo(const Options& p) {} };"
        )
        self.assertEqual(qualify("const Options&", model, "Foo"), "const Foo::Options&")
        self.assertEqual(
            qualify("const Options&", model, "Foo", expand_aliases=True), "const Foo::Param&"
        )

    def test_template_parameters_are_substituted(self):
        generic = interface(
            "template<class T> class Foo { public: Foo(typename T::Value value) {} };"
        )
        self.assertEqual(
            qualify("typename T::Value", generic, "Foo<Options>", {"T": "Options"}),
            "typename Options::Value",
        )
        model = interface(
            "template<typename T, unsigned N = 4> class Foo { public: Foo(T x = T{}) {} };"
        )
        self.assertEqual(template_bindings(model, ["float"]), {"T": "float", "N": "4"})
        self.assertEqual(template_bindings(model, ["float", "8"]), {"T": "float", "N": "8"})
        with self.assertRaisesMessage(ValueError, "Template argument Foo.T must be specified"):
            template_bindings(model, [])

    def test_constructors_list_dependencies_before_configuration(self):
        model = interface(
            "class Foo { public: Foo(int count = 10, GPIO& gpio); Foo(float gain = 1.0f); };"
        )
        self.assertEqual(
            [[p["name"] for p in c["arguments"]] for c in compliant_constructors(model)], [["gain"]]
        )
        broken = interface("class Foo { public: Foo(int count = 10, GPIO& gpio); };")
        with self.assertRaisesMessage(
            ValueError,
            "Foo: no compliant constructor; line 1: gpio: dependency without a default appears "
            "after value configuration",
        ):
            compliant_constructors(broken)

    def test_constructor_selection_uses_names_then_explicit_types(self):
        model = interface(
            "class Foo { public: Foo(int value = 0) {} Foo(float value = 0.0f) {} Foo(Port& port) {} };"
        )
        for expression, expected in (
            ("7", "int"),
            ("1.0f", "float"),
            ("static_cast<float>(Read())", "float"),
            ("int{7}", "int"),
        ):
            with self.subTest(expression=expression):
                chosen = constructor_for(model, [{"value": expression}], {}, "Foo", {})
                self.assertEqual(chosen["arguments"][0]["type"], expected)
        self.assertEqual(
            constructor_for(model, [{"port": "uart"}], {"uart": "LibXR::UART"}, "Foo", {})[
                "arguments"
            ][0]["name"],
            "port",
        )
        with self.assertRaisesMessage(
            ValueError, "Foo: constructor is ambiguous for the supplied names and explicit types"
        ):
            constructor_for(model, [{"value": "Read()"}], {}, "Foo", {})
        with self.assertRaisesMessage(
            ValueError,
            "named arguments (gain) do not match any constructor of Foo; expected one of: "
            "(value) | (value) | (port)",
        ):
            constructor_for(model, [{"gain": "1"}], {}, "Foo", {})

    def test_pointer_candidates_are_bare_names_and_selection_takes_the_address(self):
        """指针参数的候选是裸名；构造函数选择按取地址后的类型匹配裸名，'&名字' 仍兼容。
        The candidates of a pointer parameter are bare names; constructor selection matches a
        bare name by the type after taking its address, and '&name' still works.
        """
        self.assertEqual(
            binding_candidates(None, "LED*", {"led": "LED", "ledp": "LED*", "other": "Other"}),
            ["led", "ledp", "nullptr"],
        )
        model = interface("class Foo { public: Foo(LED* led) {} Foo(int led = 0) {} };")
        for value in ("led", "&led"):
            with self.subTest(value=value):
                chosen = constructor_for(model, [{"led": value}], {"led": "LED"}, "Foo", {})
                self.assertEqual(chosen["arguments"][0]["type"], "LED*")

    def test_type_shape_splits_only_outer_declarators(self):
        self.assertEqual(
            type_shape("const std::array<const GPIO*, 2>&"),
            ("std::array<constGPIO*,2>", frozenset({"const"}), (), "&"),
        )
        self.assertEqual(
            type_shape("const int* volatile* const&"),
            ("int", frozenset({"const"}), (frozenset({"volatile"}), frozenset({"const"})), "&"),
        )
        self.assertEqual(type_shape("Foo&&")[3], "&&")

    def test_dependencies_are_references_or_pointers_without_defaults(self):
        self.assertTrue(is_dependency(parameter("Port& port")))
        self.assertTrue(is_dependency(parameter("Port* port")))
        self.assertFalse(is_dependency(parameter("Port* port = nullptr")))
        self.assertFalse(is_dependency(parameter("int count")))
        self.assertFalse(is_dependency(parameter("const Param& param = {}")))

    def test_arithmetic_types(self):
        for cpp_type in (
            "int",
            "const unsigned long long",
            "std::uint8_t",
            "uint8_t",
            "bool",
            "double",
            "size_t",
        ):
            with self.subTest(cpp_type=cpp_type):
                self.assertTrue(is_arithmetic(cpp_type))
        for cpp_type in ("float&", "int*", "Mode", "std::string", "const char*"):
            with self.subTest(cpp_type=cpp_type):
                self.assertFalse(is_arithmetic(cpp_type))

    def test_explicit_expression_types(self):
        cases = {
            "true": "bool",
            "7": "int",
            "70000": None,
            "1.0f": "float",
            "1.0": "double",
            "1.0L": "long double",
            "static_cast<float>(x)": "float",
            "int{7}": "int",
            "Foo<int>{1}": "Foo<int>",
            "Read()": None,
        }
        for value, expected in cases.items():
            with self.subTest(value=value):
                self.assertEqual(explicit_expression_type(value), expected)

    def test_argument_text_rule(self):
        # 只有几个构造函数能接受这个调用时才转换。
        # A value is converted only when several constructors could take the call.
        self.assertEqual(argument_text("1", "float", False, False), "1")
        self.assertEqual(argument_text("x", "const Port&", False, False), "x")
        self.assertEqual(
            argument_text("1", "const float", False, True), "xrobot_generated::Implicit<float>(1)"
        )
        self.assertEqual(
            argument_text("x", "const Port&", False, True),
            "xrobot_generated::Implicit<const Port&>(x)",
        )
        self.assertEqual(
            argument_text("&x", "Port*", False, True), "xrobot_generated::Implicit<Port*>(&x)"
        )
        self.assertEqual(argument_text("x", "Port&&", False, True), "static_cast<Port&&>(x)")
        self.assertEqual(argument_text("v", "Mode", False, True), "static_cast<Mode>(v)")
        self.assertEqual(argument_text("Make()", "Config", True, True), "Make()")
        # 花括号总是带上类型，不论是否转换。
        # A brace initializer always carries its type.
        self.assertEqual(argument_text("{1}", "const Foo&", False, False), "Foo{1}")
        self.assertEqual(
            argument_text("{1, 2}", "std::array<int, 2>", False, True), "std::array<int, 2>{1, 2}"
        )
