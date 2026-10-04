"""从模块头文件提取构造接口（xrobot.source_syntax）。
Extracting the constructor interface from a Module header (xrobot.source_syntax).
"""

from fixtures import TestCase

from xrobot.constructor_model import (
    enrich_interface,
)
from xrobot.source_syntax import extract_interface, parameter_arity, viable_constructors


def interface(source, name="Foo"):
    """头文件文本中一个类的构造接口，含参数形状。
    The constructor interface of a class in a header text, with parameter shapes.
    """
    return enrich_interface(source, extract_interface(source, name))


class InterfaceExtraction(TestCase):
    """哪些类和构造函数构成模块的接口。
    Which class and constructors make up a Module's interface.
    """

    def test_copy_and_move_constructors_are_not_part_of_the_interface(self):
        for extra in (
            "Foo(const Foo&) = default;",
            "Foo(const Foo& other) = default;",
            "Foo(Foo&&) noexcept = default;",
            "Foo(Foo const&) = default;",
        ):
            with self.subTest(extra=extra):
                result = interface(f"class Foo {{ public: Foo(int n = 1) {{}} {extra} }};")
                self.assertEqual([c["parameters"] for c in result["constructors"]], [["int n = 1"]])
        result = interface(
            "template <typename T> class Foo { public: Foo(T n = {}) {} "
            "Foo(const Foo<T>&) = default; };"
        )
        self.assertEqual([c["parameters"] for c in result["constructors"]], [["T n = {}"]])
        with self.assertRaisesMessage(
            ValueError, "No supported explicit public constructor for Foo"
        ):
            extract_interface("class Foo { public: Foo(const Foo&) = default; };", "Foo")
        # 参数类型不是本类的单参数构造函数照常保留。
        # A one-parameter constructor of another type stays.
        result = interface("class Foo { public: Foo(const FooConfig& config) {} Foo(Foo* p) {} };")
        self.assertEqual(
            [c["parameters"] for c in result["constructors"]],
            [["const FooConfig& config"], ["Foo* p"]],
        )

    def test_the_argument_counts_of_every_constructor_are_recorded(self):
        # 私有和已删除的构造函数也参与重载决议，所以也记录；拷贝和移动构造除外。
        # Private and deleted constructors take part in overload resolution, so they are
        # recorded too; copy and move constructors are not.
        result = extract_interface(
            "class Foo { public:\n  Foo() {}\n  Foo(int a, float b = 1.0f) {}\n"
            "  Foo(const char* name, ...) {}\n  Foo(const Foo&) = default;\n  Foo(long) = delete;\n"
            " private:\n  Foo(void* p, int n, int m = 0) {}\n};",
            "Foo",
        )
        self.assertEqual(result["arities"], [(0, 0), (1, 2), (1, None), (1, 1), (2, 3)])
        self.assertEqual(
            [viable_constructors(result["arities"], count) for count in range(5)], [1, 3, 3, 2, 1]
        )

    def test_parameter_arity_counts_defaults_and_packs(self):
        self.assertEqual(parameter_arity([]), (0, 0))
        self.assertEqual(parameter_arity(["void"]), (0, 0))
        self.assertEqual(parameter_arity(["int a", "std::array<int, 2> b = {}"]), (1, 2))
        self.assertEqual(parameter_arity(["int a", "Args&&... rest"]), (1, None))

    def test_template_declarations_are_listed_one_by_one(self):
        result = interface(
            "template <typename T = std::pair<int, int>, int N = 2>\n"
            "class Foo { public: Foo(T value = {}) {} };"
        )
        self.assertEqual(
            result["template_declarations"], ["typename T = std::pair<int, int>", "int N = 2"]
        )
        self.assertEqual([p["name"] for p in result["template_parameters"]], ["T", "N"])

    def test_a_class_outside_global_scope_names_its_scope(self):
        for source, scope in (
            ("namespace team { class Foo { public: Foo() {} }; }", "namespace team"),
            ("namespace { class Foo { public: Foo() {} }; }", "an anonymous namespace"),
            ("class Outer { class Foo { public: Foo() {} }; };", "class Outer"),
            ("void f() { class Foo { public: Foo() {} }; }", "a function body"),
        ):
            with (
                self.subTest(source=source),
                self.assertRaisesMessage(
                    ValueError,
                    f"Foo is declared inside {scope}; a Module class must be declared at global "
                    "scope",
                ),
            ):
                extract_interface(source, "Foo")
        with self.assertRaisesMessage(ValueError, "No global class Foo is declared in this header"):
            extract_interface("class Bar { public: Bar() {} };", "Foo")
