"""生成的头文件与 LibXR 的 .clang-format 一致：固定版本的 clang-format 不改动它。
Generated headers match LibXR's .clang-format: a pinned clang-format changes nothing.

没有安装这个版本的 clang-format（pip install clang-format==21.1.8）时跳过。
The tests are skipped without that version of clang-format (pip install clang-format==21.1.8).
"""

import shutil
import subprocess
import unittest

from test_generate_main import GenerationTestCase, led, probe

from xrobot.generate_main import generate

PINNED_VERSION = "21.1.8"
# LibXR 的 .clang-format：Google 风格，列宽 90，Allman 花括号，重新分组 include。
# LibXR's .clang-format: Google style, column limit 90, Allman braces, regrouped includes.
STYLE = (
    "{Language: Cpp, BasedOnStyle: Google, IncludeBlocks: Regroup, ColumnLimit: 90, "
    "BreakBeforeBraces: Allman}"
)


def clang_format():
    """固定版本的 clang-format 的路径；没有时为 None。
    The path of the pinned clang-format, or None without it.
    """
    path = shutil.which("clang-format")
    if path is None:
        return None
    result = subprocess.run([path, "--version"], capture_output=True, text=True, timeout=60)
    return path if PINNED_VERSION in result.stdout else None


CLANG_FORMAT = clang_format()

VALUES = """#include <initializer_list>
class Values { public:
  struct Param {
    struct Limits { float low; float high; int steps; };
    struct Gains { float kp, ki, kd; };
    Limits limits; Gains gains; int count; const char* name; };
  struct Item { const char* label; int id; };
  Values(Port& port, const Param& param, std::initializer_list<Item> items,
         std::initializer_list<const char*> names, int rate = 100) {}
  void OnMonitor() {} };"""
PICK = """#include "Led.hpp"
class Pick { public:
  Pick(Port& port, int n = 1) {}
  Pick(Led& led, int n = 1) {} };"""
RECORD = """struct RecordParam { int a; float b; };
class Record { public: explicit Record(RecordParam param = {}) {} };"""
AUTO = "class Auto { public: Auto() {} auto OnMonitor() {} };"
NAMES = ["alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta"]


@unittest.skipIf(
    CLANG_FORMAT is None, f"clang-format {PINNED_VERSION} is not installed (pip install it)"
)
class GeneratedHeadersNeedNoFormatting(GenerationTestCase):
    """用 LibXR 的风格格式化生成的头文件，结果与原文相同。
    Formatting a generated header in LibXR's style gives the same text.
    """

    def setUp(self):
        super().setUp()
        self.module("Values", VALUES)
        self.module("Pick", PICK)
        self.module("Record", RECORD)
        self.module("Auto", AUTO)

    def assert_formatted(self, text):
        """断言 clang-format 不改动 text，有或没有 #line 都一样。
        Assert that clang-format changes nothing in text.
        """
        result = subprocess.run(
            [CLANG_FORMAT, f"--style={STYLE}", "--assume-filename=xrobot_main.hpp"],
            input=text,
            capture_output=True,
            text=True,
            encoding="utf-8",
            timeout=120,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(text.split("\n"), result.stdout.split("\n"))

    def check(self, name="xrobot.yaml", **top):
        """生成一份配置的头文件，分别带和不带 #line 检查。
        Generate the header of a configuration and check it with and without #line.
        """
        path = self.config(top, name=name)
        for directives in (True, False):
            self.assert_formatted(generate(self.project, path, line_directives=directives))

    def test_instances_with_struct_values_and_dependencies(self):
        self.check(
            modules=[led("a"), led("b", gpio="uart", param={"cycle": "5", "inverted": "true"})]
        )
        self.check(
            modules=[
                led("first"),
                probe("p", port="sub", optional="&port", count="7", name='"a longer name"'),
                {"module": "Cmd", "id": "cmd", "args": [{"led": "first"}, {"backup": "&first"}]},
            ]
        )

    def test_many_registrations_wrap_the_entry_function_and_the_macro(self):
        for count in (14, 30):
            names = [f"gpio_{i}" for i in range(count)]
            self.entry(
                '#include "xrobot_main.hpp"\nint main() {\n'
                + "".join(f"  XR_REGISTER({name}, LibXR::GPIO);\n" for name in names)
                + "  XROBOT_MAIN();\n}\n"
            )
            self.check(modules=[led(f"led_{i}", gpio=name) for i, name in enumerate(names)])

    def test_nested_values_lists_and_long_declarations(self):
        items = ", ".join(f'{{"{name}", {i}}}' for i, name in enumerate(NAMES))
        self.check(
            modules=[
                {
                    "module": "Values",
                    "id": "values",
                    "args": [
                        {"port": "port"},
                        {
                            "param": {
                                "limits": {"low": "0.5f", "high": "99.5f", "steps": "128"},
                                "gains": {"kp": "1.0f", "ki": "0.25f", "kd": "0.001f"},
                                "count": "3",
                                "name": '"nested"',
                            }
                        },
                        {"items": "{" + items + "}"},
                        {"names": [f'"{name}"' for name in NAMES]},
                        {"rate": "250"},
                    ],
                },
                {
                    "module": "Values",
                    "id": "a_very_long_instance_identifier_for_the_second_values",
                    "args": [
                        {"port": "port"},
                        {
                            "param": {
                                "limits": {"low": "0.5f", "high": "99.5f", "steps": "128"},
                                "gains": {"kp": "1.0f", "ki": "0.25f", "kd": "0.001f"},
                                "count": "3",
                                "name": '"nested"',
                            }
                        },
                        {"items": "{{" + '"only", 1' + "}}"},
                        {"names": ['"one"']},
                        {"rate": "1"},
                    ],
                },
            ]
        )

    def test_values_with_a_list_too_long_for_one_line(self):
        long_items = ", ".join(f'{{"{name}_{i}", {i}}}' for i in range(40) for name in NAMES[:1])
        self.check(
            modules=[
                {
                    "module": "Values",
                    "id": "values",
                    "args": [
                        {"port": "port"},
                        {
                            "param": {
                                "limits": {"low": "0", "high": "1", "steps": "2"},
                                "gains": {"kp": "1", "ki": "2", "kd": "3"},
                                "count": "3",
                                "name": '"n"',
                            }
                        },
                        {"items": "{" + long_items + "}"},
                        {"names": [f'"name_{i}_with_some_length"' for i in range(30)]},
                        {"rate": "1"},
                    ],
                }
            ]
        )

    def test_constructor_selection_defines_the_conversion_helper(self):
        self.check(
            modules=[
                led("led"),
                {"module": "Pick", "id": "pick", "args": [{"port": "sub"}, {"n": "2"}]},
                {"module": "Pick", "id": "other", "args": [{"led": "led"}, {"n": "3"}]},
            ]
        )

    def test_constants_and_includes(self):
        self.check(
            modules=[led("led")],
            constexpr_namespace="Board",
            constexpr_includes=["<vector>", "Board.hpp", "<stdint.h>", "sub/Pins.h"],
            constexprs={
                "Rate": {"type": "int", "value": "250"},
                "Gains": {"type": "std::array<float, 2>", "value": "{1.0F, 2.0F}"},
                "Limits": {"type": "RecordParam", "value": {"a": "1", "b": "2.0F"}},
            },
            settings={"monitor_sleep_ms": "20"},
        )

    def test_a_long_configuration_name_wraps_the_banner_and_the_comments(self):
        # 说明行和注释会折行；// xrobot: 行不能折，所以名字只长到说明行超出列宽为止。
        # The banner and comments wrap, the // xrobot: lines cannot, so the name is long enough
        # to wrap the banner only.
        name = "products/a_rather_long_product_name_xxxx.yaml"
        self.check(name=name, modules=[led("an_instance_with_a_rather_long_identifier_as_well")])

    def test_monitor_assertions_for_results_the_index_cannot_read(self):
        long_id = "a_rather_long_instance_name"
        self.check(
            modules=[
                {"module": "Auto", "id": "brief"},
                {"module": "Auto", "id": long_id},
            ]
        )

    def test_no_instances(self):
        self.check(modules=[])
