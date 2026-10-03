"""生成的 C++ 的排版（xrobot.cpp_layout）：值、声明、调用、宏、注释和 include。
Layout of the generated C++ (xrobot.cpp_layout): values, declarations, calls, macros, comments
and includes.

期望的文本与固定版本的 clang-format 对 LibXR 风格的输出相同；test_clang_format.py 对生成的
头文件逐一核对这一点。
The expected text is what the pinned clang-format gives in LibXR's style; test_clang_format.py
checks that on generated headers.
"""

from fixtures import TestCase

from xrobot.cpp_layout import (
    flat,
    layout_call,
    layout_declaration,
    layout_macro,
    layout_value,
    parse_node,
    sort_includes,
    wrap_comment,
)


class Parsing(TestCase):
    """把 C++ 文本拆成花括号列表、调用和指定初始化项。
    Splitting C++ text into brace lists, calls and designated entries.
    """

    def test_brace_lists_calls_and_designated_entries_are_split(self):
        node = parse_node('{.a = 1, .b = {2, "x, y"}, .c = Foo<int, 2>(4, {5})}')
        self.assertEqual(node.kind, "brace")
        self.assertEqual(
            [(i.kind, i.text) for i in node.items],
            [("designated", "a"), ("designated", "b"), ("designated", "c")],
        )
        self.assertEqual(node.items[1].items[0].kind, "brace")
        self.assertEqual(node.items[2].items[0].kind, "call")
        self.assertEqual(node.items[2].items[0].text, "Foo<int, 2>")
        self.assertEqual(flat(node), '{.a = 1, .b = {2, "x, y"}, .c = Foo<int, 2>(4, {5})}')

    def test_typed_braces_keep_their_type(self):
        node = parse_node("std::array<int, 2>{1,  2}")
        self.assertEqual((node.kind, node.text), ("brace", "std::array<int, 2>"))
        self.assertEqual(flat(node), "std::array<int, 2>{1, 2}")

    def test_anything_else_is_text_as_written(self):
        for text in ("a + f(x)", "cond ? {1} : {2}", "values[0]", "(a + b) * c", "-f(1)", "x"):
            with self.subTest(text=text):
                node = parse_node(text)
                self.assertEqual((node.kind, node.text), ("text", text))


class Values(TestCase):
    """值接在声明之后的排版。
    Laying out a value after a declaration.
    """

    HEAD = "  static const T x = "

    def lay(self, text):
        """HEAD 之后的一个值的排版结果。
        The layout of a value after HEAD.
        """
        return layout_value(self.HEAD, text, ";", 2)

    def test_a_value_that_fits_stays_on_one_line(self):
        self.assertEqual(self.lay("{1, 2}"), ["  static const T x = {1, 2};"])
        self.assertEqual(self.lay("Param{1, 2}"), ["  static const T x = Param{1, 2};"])

    def test_an_outermost_struct_has_one_field_per_line_even_when_it_fits(self):
        self.assertEqual(
            self.lay("{.a = 1, .b = {.c = 2}}"),
            ["  static const T x = {", "      .a = 1,", "      .b = {.c = 2},", "  };"],
        )
        self.assertEqual(self.lay("{}"), ["  static const T x = {};"])

    def test_a_nested_struct_that_does_not_fit_breaks_after_its_field_name(self):
        text = (
            '{.name = "n", .inner = {.first_long_field = 1000000, '
            ".second_long_field = 2000000, .third_long_field = 3}}"
        )
        self.assertEqual(
            self.lay(text),
            [
                "  static const T x = {",
                '      .name = "n",',
                "      .inner =",
                "          {",
                "              .first_long_field = 1000000,",
                "              .second_long_field = 2000000,",
                "              .third_long_field = 3,",
                "          },",
                "  };",
            ],
        )

    def test_a_list_of_five_or_more_fills_columns(self):
        text = "{" + ", ".join(f'"name_{i}"' for i in range(12)) + "}"
        self.assertEqual(
            self.lay(text),
            [
                "  static const T x = {",
                '      "name_0", "name_1", "name_2", "name_3", "name_4",  "name_5",',
                '      "name_6", "name_7", "name_8", "name_9", "name_10", "name_11",',
                "  };",
            ],
        )

    def test_a_list_of_fewer_items_has_one_per_line(self):
        text = "{" + ", ".join(f"Some::Long::Qualified::Name{i}" for i in range(4)) + "}"
        lines = self.lay(text)
        self.assertEqual(len(lines), 6)
        self.assertEqual(lines[1], "      Some::Long::Qualified::Name0,")

    def test_a_call_fills_lines_aligned_after_its_parenthesis(self):
        text = "Type(" + ", ".join(f"argument_number_{i}" for i in range(8)) + ")"
        self.assertEqual(
            self.lay(text),
            [
                "  static const T x = Type(argument_number_0, argument_number_1, argument_number_2,",
                "                          argument_number_3, argument_number_4, argument_number_5,",
                "                          argument_number_6, argument_number_7);",
            ],
        )

    def test_text_that_cannot_be_split_overflows_instead_of_breaking(self):
        long_text = "a_very_long_name_" * 8
        self.assertEqual(self.lay(long_text), [f"{self.HEAD}{long_text};"])


class Declarations(TestCase):
    """第一行放不下的声明。
    Declarations whose first line is too long.
    """

    def test_a_long_name_goes_to_the_next_line_after_the_type(self):
        self.assertEqual(
            layout_declaration(
                "static const std::initializer_list<SharedTopic::TopicConfig>",
                "xr_shared_topic_topic_configs",
                '{"a", "b"}',
                ";",
                2,
            ),
            [
                "  static const std::initializer_list<SharedTopic::TopicConfig>",
                '      xr_shared_topic_topic_configs = {"a", "b"};',
            ],
        )

    def test_a_brace_that_does_not_fit_after_the_name_starts_its_own_line(self):
        items = ", ".join(f"Entry::kValue{i}" for i in range(7))
        self.assertEqual(
            layout_declaration(
                "static const std::initializer_list<EventBinder::ModuleInfo>",
                "xr_jftjjislhhdxmel_ltqslbq",
                "{" + items + "}",
                ";",
                2,
            ),
            [
                "  static const std::initializer_list<EventBinder::ModuleInfo> "
                "xr_jftjjislhhdxmel_ltqslbq =",
                "      {",
                "          Entry::kValue0, Entry::kValue1, Entry::kValue2, Entry::kValue3,",
                "          Entry::kValue4, Entry::kValue5, Entry::kValue6,",
                "      };",
            ],
        )


class Calls(TestCase):
    """调用、函数声明和宏的实参排版。
    The argument layout of calls, function declarations and macros.
    """

    def test_arguments_that_fit_share_one_line(self):
        self.assertEqual(
            layout_call("static CMD cmd(", ['"a"', "b"], ");", 2), ['  static CMD cmd("a", b);']
        )

    def test_a_call_aligns_after_the_parenthesis(self):
        args = [f"motor_{i}" for i in range(9)]
        self.assertEqual(
            layout_call("static Chassis<Mecanum> chassis(", args, ");", 2),
            [
                "  static Chassis<Mecanum> chassis(motor_0, motor_1, motor_2, motor_3, motor_4, "
                "motor_5,",
                "                                  motor_6, motor_7, motor_8);",
            ],
        )

    def test_a_call_breaks_after_the_parenthesis_when_that_saves_a_line(self):
        args = [f"argument_{i:02d}" for i in range(14)]
        lines = layout_call("static Something_long_name instance_with_long_name(", args, ");", 2)
        self.assertEqual(lines[0], "  static Something_long_name instance_with_long_name(")
        self.assertEqual(lines[1][:10], "      argu")
        self.assertTrue(all(len(line) <= 90 for line in lines))

    def test_a_function_declaration_is_more_reluctant_to_break_after_the_parenthesis(self):
        params = [f"LibXR::GPIO& gpio_{i}" for i in range(6)]
        head = "[[noreturn]] static inline void XRobotMain("
        aligned = layout_call(head, params, ")", 0, hang_margin=3)
        self.assertEqual(len(aligned), 3)
        self.assertTrue(aligned[1].startswith(" " * 43 + "LibXR::GPIO& gpio_2"))
        many = [f"LibXR::GPIO& gpio_{i}" for i in range(30)]
        hanging = layout_call(head, many, ")", 0, hang_margin=3)
        self.assertEqual(hanging[0], head)
        self.assertTrue(hanging[1].startswith("    LibXR::GPIO& gpio_0, "))

    def test_a_short_macro_is_one_line_and_a_long_one_aligns_its_continuations(self):
        self.assertEqual(
            layout_macro("XROBOT_MAIN", "::XRobotMain", ["a", "b"]),
            ["#define XROBOT_MAIN() ::XRobotMain(a, b)"],
        )
        lines = layout_macro("XROBOT_MAIN", "::XRobotMain", [f"gpio_{i}" for i in range(14)])
        self.assertEqual(
            lines,
            [
                "#define XROBOT_MAIN()" + " " * 66 + "\\",
                "  ::XRobotMain(gpio_0, gpio_1, gpio_2, gpio_3, gpio_4, gpio_5, gpio_6, gpio_7, "
                "gpio_8, \\",
                "               gpio_9, gpio_10, gpio_11, gpio_12, gpio_13)",
            ],
        )


class Text(TestCase):
    """注释和 include。
    Comments and includes.
    """

    def test_a_long_comment_is_broken_at_spaces(self):
        text = (
            "Generated by `xrobot gen` from User/products/a_rather_long_product_name_xxxx.yaml; "
            "do not edit by hand."
        )
        self.assertEqual(
            wrap_comment(text),
            [
                "// Generated by `xrobot gen` from User/products/a_rather_long_product_name_xxxx"
                ".yaml; do",
                "// not edit by hand.",
            ],
        )
        self.assertEqual(wrap_comment("short", 2), ["  // short"])

    def test_includes_are_grouped_and_sorted_like_clang_format_regroups_them(self):
        self.assertEqual(
            sort_includes(
                ['"libxr.hpp"', "<vector>", '"Led.hpp"', "<stdint.h>", '"sub/Pins.h"', '"Led.hpp"']
                + ["<array>"]
            ),
            [
                "#include <stdint.h>",
                "",
                "#include <array>",
                "#include <vector>",
                "",
                '#include "Led.hpp"',
                '#include "libxr.hpp"',
                '#include "sub/Pins.h"',
            ],
        )
