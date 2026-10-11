"""读取和检查应用配置（xrobot.config）。
Reading and checking application configurations (xrobot.config).
"""

import unittest

from fixtures import TempDirTestCase, TestCase

from xrobot.config import (
    ConfigError,
    cpp_string_literal,
    identifier_problem,
    load_config,
    parse_yaml,
    scalar_style,
    string_literal_content,
    validate_config,
    value_text,
)


def load(text, source="cfg.yaml"):
    """读取并检查配置文本。
    Read and check a configuration text.
    """
    config = parse_yaml(text, source)
    validate_config(config, source)
    return config


def instance(**values):
    """一个 Foo 实例的配置文本，参数按给出的顺序。
    The configuration text of one Foo instance with the given arguments in order.
    """
    return "modules:\n  - module: Foo\n    id: foo\n    args:\n" + "".join(
        f"      - {name}: {value}\n" for name, value in values.items()
    )


class Scalars(TestCase):
    """标量值如何读成 C++ 文本。
    How scalar values are read as C++ text.
    """

    def test_scalars_keep_their_cpp_spelling(self):
        config = load(
            "modules:\n- module: Foo\n  id: foo\n  args: [{a: on}, {b: off}, {c: yes}, {d: no},"
            " {e: 0xFF}, {f: 1e-3}, {g: true}, {h: '\"hello\"'}, {i: 1.0F}]\n"
        )
        self.assertEqual(
            config["modules"][0]["args"],
            [
                {"a": "on"},
                {"b": "off"},
                {"c": "yes"},
                {"d": "no"},
                {"e": "0xFF"},
                {"f": "1e-3"},
                {"g": "true"},
                {"h": '"hello"'},
                {"i": "1.0F"},
            ],
        )

    def test_double_quoted_values_are_cpp_string_literals(self):
        config = load(
            'modules:\n- module: Foo\n  id: foo\n  args:\n    - a: "bmi088_gyro"\n'
            '    - b: "say \\"hi\\"\\tC:\\\\x"\n    - c: "中文"\n    - d: ""\n'
            '    - e: "null"\n    - f: "\\x01"\n'
        )
        self.assertEqual(
            config["modules"][0]["args"],
            [
                {"a": '"bmi088_gyro"'},
                {"b": '"say \\"hi\\"\\tC:\\\\x"'},
                {"c": '"中文"'},
                {"d": '""'},
                {"e": '"null"'},
                {"f": '"\\001"'},
            ],
        )

    def test_single_quoted_values_are_code_as_written(self):
        config = load(instance(a="'{0.707, 0.0}'", b="'&ref'", c="'\"x\"'", d="'{}'"))
        self.assertEqual(
            config["modules"][0]["args"],
            [{"a": "{0.707, 0.0}"}, {"b": "&ref"}, {"c": '"x"'}, {"d": "{}"}],
        )

    def test_string_literals_round_trip_through_their_content(self):
        for content in ("bmi088_gyro", 'a"b', "tab\there", "nl\nx", "back\\slash", "中文", ""):
            with self.subTest(content=content):
                self.assertEqual(string_literal_content(cpp_string_literal(content)), content)
        for text in ('"a" "b"', '"\\x41"', 'u8"a"', "x"):
            with self.subTest(text=text):
                self.assertIsNone(string_literal_content(text))

    def test_canonical_quoting_of_values(self):
        cases = [
            ("250", None, None),
            ("LED_B", None, None),
            ("BMI088::GyroRange::DEG_2000DPS", None, None),
            ("LibXR::Terminal<32, 32>", None, "'"),
            ("-1", None, None),
            ("nullptr", None, None),
            ("true", None, None),
            ('"bmi088_gyro"', '"', '"'),
            ("{0.707, 0.0}", "'", "'"),
            ("{}", "'", "'"),
            ("&ref", "'", "'"),
            ("null", "'", "'"),
            ("a: b", "'", "'"),
            ('"a" "b"', "'", "'"),
        ]
        for text, block, flow in cases:
            with self.subTest(text=text):
                self.assertEqual(scalar_style(text), block)
                self.assertEqual(scalar_style(text, flow=True), flow)

    def test_null_tilde_and_empty_mean_not_filled_but_quoted_null_is_text(self):
        config = load(instance(a="null", b="~", c="", d="'null'", e="nullptr", f="NULL"))
        self.assertEqual(
            config["modules"][0]["args"],
            [{"a": None}, {"b": None}, {"c": None}, {"d": "null"}, {"e": "nullptr"}, {"f": None}],
        )

    def test_unfilled_values_are_a_valid_saved_configuration(self):
        load(instance(a="null"))

    def test_unfilled_value_is_rejected_when_its_text_is_needed(self):
        with self.assertRaisesMessage(
            ConfigError,
            (
                'foo.args.a is not filled in (null, ~ and empty values mean "not filled in"; '
                "write nullptr for a null pointer)"
            ),
        ):
            value_text(None, "foo.args.a")

    def test_source_lines_are_recorded_for_keys_and_list_items(self):
        config = parse_yaml(
            "# header\nmodules:\n  - module: Foo\n    id: foo\n    args:\n"
            "      - a: 1\n      - b: 2\n",
            "cfg.yaml",
        )
        self.assertEqual(config.key_lines["modules"], 2)
        entry = config["modules"][0]
        self.assertEqual(entry.line, 3)
        self.assertEqual(entry.key_lines["id"], 4)
        self.assertEqual(entry["args"].item_lines, [6, 7])

    def test_empty_document_is_an_empty_configuration(self):
        self.assertEqual(load(""), {})


class YamlFeatures(TestCase):
    """配置中不允许的 YAML 写法和语法错误。
    The YAML features a configuration rejects, and syntax errors.
    """

    def test_anchors_and_aliases_are_rejected_with_a_hint(self):
        for text, line in (
            ("a: &x 1\nb: *x\n", 1),
            ("modules:\n  - &first {module: Foo, id: foo}\n", 2),
        ):
            with (
                self.subTest(text=text),
                self.assertRaisesMessage(
                    ConfigError,
                    f"cfg.yaml:{line}: YAML anchors and aliases are not allowed; reference "
                    "instances by id and share values through constexprs",
                ),
            ):
                load(text)

    def test_an_anchor_without_a_value_suggests_the_bare_name(self):
        """跟着旧提示写的 ``gpio: &led`` 单独说明是空锚点，建议去掉 & 只写名字。
        A ``gpio: &led`` line written after the old hint gets its own message about the empty
        anchor, suggesting the bare name without &.
        """
        with self.assertRaisesMessage(
            ConfigError,
            "cfg.yaml:1: &led makes the value of gpio an empty YAML anchor; write the name "
            "without &, e.g. `gpio: led`",
        ):
            load("gpio: &led\n")
        # 有值的锚点、映射上的锚点、别名和不带键的值仍是原来的提示。
        # An anchor with a value, one on a mapping, an alias and a keyless value keep the
        # original hint.
        for text, line in (
            ("a: &x 1\nb: *x\n", 1),
            ("modules:\n  - &first {module: Foo, id: foo}\n", 2),
            ("a: &x\n  b: 1\n", 1),
            ("&x\n", 1),
        ):
            with (
                self.subTest(text=text),
                self.assertRaisesMessage(
                    ConfigError,
                    f"cfg.yaml:{line}: YAML anchors and aliases are not allowed; reference "
                    "instances by id and share values through constexprs",
                ),
            ):
                load(text)

    def test_tags_are_rejected(self):
        for text, where in (
            ("modules: !!seq []\n", "1: YAML tags (tag:yaml.org,2002:seq)"),
            ("settings:\n  monitor_sleep_ms: !custom 10\n", "2: YAML tags (!custom)"),
            (
                "constexprs: {a: {type: int, value: !!str '1'}}\n",
                "1: YAML tags (tag:yaml.org,2002:str)",
            ),
        ):
            with (
                self.subTest(text=text),
                self.assertRaisesMessage(
                    ConfigError, f"cfg.yaml:{where} are not allowed; write the value as C++ text"
                ),
            ):
                load(text)

    def test_duplicate_keys_are_rejected_with_their_line(self):
        with self.assertRaisesMessage(ConfigError, "cfg.yaml:2: duplicate key modules"):
            load("modules: []\nmodules: []\n")
        with self.assertRaisesMessage(ConfigError, "cfg.yaml:4: duplicate key id"):
            load("modules:\n  - module: Foo\n    id: a\n    id: b\n")

    def test_complex_mapping_keys_are_rejected(self):
        with self.assertRaisesMessage(ConfigError, "cfg.yaml:1: mapping keys must be plain names"):
            load("{a: 1}: 2\n")

    def test_syntax_errors_carry_file_and_line(self):
        with self.assertRaisesMessage(
            ConfigError, "cfg.yaml:3: YAML syntax error: expected <block end>, but found '?'"
        ):
            load("modules:\n  - module: Foo\n  id: [\n")


class CppText(TestCase):
    """值中 C++ 文本的检查：注释、八进制和数字。
    Checks of the C++ text in values: comments, octal and digits.
    """

    def test_cpp_comments_inside_a_value_are_rejected(self):
        for value in ("1 // tuned", "'1 /* tuned */'"):
            with (
                self.subTest(value=value),
                self.assertRaisesMessage(
                    ConfigError,
                    "cfg.yaml: foo.args.a: C++ comments are not allowed inside a value; use a "
                    "YAML # comment",
                ),
            ):
                load(instance(a=value))

    def test_comment_markers_inside_string_literals_are_text(self):
        load(instance(a="'\"http://x\"'", b="'\"/*\"'"))

    def test_leading_zero_integers_are_rejected_as_octal(self):
        for value, number in (
            ("010", "010"),
            ("00", "00"),
            ("'0010'", "0010"),
            ("017u", "017u"),
            ("'f(07)'", "07"),
        ):
            with (
                self.subTest(value=value),
                self.assertRaisesMessage(
                    ConfigError,
                    f"cfg.yaml: foo.args.a: {number} has a leading zero, which C++ reads as "
                    "octal; write the decimal value",
                ),
            ):
                load(instance(a=value))

    def test_zero_hex_and_decimal_fractions_are_accepted(self):
        load(instance(a="0", b="0x10", c="0.5", d="0b101", e="1e-3", f="10"))

    def test_non_ascii_digits_are_rejected(self):
        for value in ("１２", "'f(٣)'"):
            with (
                self.subTest(value=value),
                self.assertRaisesMessage(
                    ConfigError, "cfg.yaml: foo.args.a: non-ASCII digits are not C++ numbers"
                ),
            ):
                load(instance(a=value))

    def test_at_syntax_is_rejected(self):
        with self.assertRaisesMessage(
            ConfigError,
            "cfg.yaml: foo.args.a: the @ prefix of XRobot before 1.0 is gone; every value "
            "without quotes or in single quotes is C++ code, and a double-quoted value is a "
            "C++ string",
        ):
            load(instance(a="'@nullptr'"))


class Identifiers(TestCase):
    """生成的 C++ 名字（实例 id、常量名、命名空间）的规则。
    The rules for generated C++ names (instance ids, constant names, namespaces).
    """

    def test_identifier_rules(self):
        cases = {
            "led": None,
            "motor_1": None,
            "_private": None,
            "Chassis2": None,
            "xrobotic": None,
            "XRay": None,
            "1led": "is not a C++ identifier",
            "a-b": "is not a C++ identifier",
            "": "is not a C++ identifier",
            "class": "is a C++ keyword",
            "and": "is a C++ keyword",
            "final": "is a C++ keyword",
            "co_await": "is a C++ keyword",
            "ASSERT": "is a macro name",
            "NULL": "is a macro name",
            "XR_REGISTER": "is a macro name",
            "assert": "is a macro name",
            "std": "is a reserved namespace",
            "LibXR": "is a reserved namespace",
            "xrobot_generated": "is a reserved namespace",
            "xr_led": "uses a prefix reserved for generated names",
            "XR_LED": "uses a prefix reserved for generated names",
            "xrobot_led": "uses a prefix reserved for generated names",
            "__led": "is reserved by the C++ standard",
            "_Led": "is reserved by the C++ standard",
        }
        for name, problem in cases.items():
            with self.subTest(name=name):
                self.assertEqual(identifier_problem(name), problem)

    def test_invalid_instance_ids_are_rejected(self):
        for identity in ("CMD2 x", "ASSERT", "while", "xr_led"):
            with (
                self.subTest(identity=identity),
                self.assertRaisesMessage(
                    ConfigError,
                    f"cfg.yaml: modules[0].id: {identity} {identifier_problem(identity)}",
                ),
            ):
                load(f"modules:\n  - module: Foo\n    id: {identity}\n")


class Structure(TestCase):
    """配置的结构：顶层键、实例、参数、常量和设置。
    The structure of a configuration: top-level keys, instances, arguments, constants and
    settings.
    """

    def test_unknown_top_level_keys_are_rejected(self):
        for key in ("instances", "constexpr"):
            with (
                self.subTest(key=key),
                self.assertRaisesMessage(
                    ConfigError,
                    f"cfg.yaml: unknown top-level key(s) {key}; allowed: modules, settings, "
                    "constexprs, constexpr_namespace, constexpr_includes",
                ),
            ):
                load(f"{key}: {{}}\n")

    def test_unknown_instance_keys_are_rejected(self):
        for key in ("depends", "template"):
            with (
                self.subTest(key=key),
                self.assertRaisesMessage(
                    ConfigError, f"cfg.yaml: modules[0]: unknown key(s) {key}"
                ),
            ):
                load(f"modules:\n  - module: Foo\n    id: foo\n    {key}: []\n")

    def test_configurations_of_xrobot_before_1_0_get_one_error(self):
        for text in (
            "global_settings:\n  monitor_sleep_ms: 1000\nmodules: []\n",
            "modules:\n  - name: BlinkLED\n    constructor_args:\n      blink_cycle: 250\n",
            "modules:\n  - module: Foo\n    id: foo\n    constructor_args: {}\n",
        ):
            with self.subTest(text=text), self.assertRaises(ConfigError) as context:
                load(text)
            self.assertEqual(
                str(context.exception),
                "cfg.yaml: this configuration uses the format of XRobot before 1.0 "
                "(global_settings, name/constructor_args); xrobot 1.0 lists each instance as "
                "module, id and args; replace the content of the file with `modules: []` (or "
                "delete the file and run `xrobot init`), then recreate the instances with "
                "`xrobot instance -c cfg.yaml add`",
            )

    def test_instances_need_module_and_id(self):
        with self.assertRaisesMessage(ConfigError, "cfg.yaml: modules[0].module is required"):
            load("modules:\n  - id: foo\n")
        with self.assertRaisesMessage(ConfigError, "cfg.yaml: modules[0].id is required"):
            load("modules:\n  - module: Foo\n")

    def test_duplicate_instance_ids_are_rejected(self):
        with self.assertRaisesMessage(
            ConfigError, "cfg.yaml: modules[1].id: duplicate instance id foo"
        ):
            load("modules:\n  - {module: Foo, id: foo}\n  - {module: Foo, id: foo}\n")

    def test_arguments_are_an_ordered_list_of_single_named_values(self):
        cases = [
            ("args: {a: 1}", "foo.args must be an ordered list"),
            ("args: [{a: 1, b: 2}]", "foo.args[0] requires one named parameter"),
            ("args: [1]", "foo.args[0] requires one named parameter"),
            ("args: [{a: 1}, {a: 2}]", "foo.args[1]: invalid or duplicate parameter name a"),
            ("template_args: int", "foo.template_args must be an ordered list"),
        ]
        for text, message in cases:
            with (
                self.subTest(text=text),
                self.assertRaisesMessage(ConfigError, "cfg.yaml: " + message),
            ):
                load(f"modules:\n  - module: Foo\n    id: foo\n    {text}\n")

    def test_nested_values_are_checked_with_their_path(self):
        # 名字无效的字段不再检查它的值（'1x' 的 010 不单独报错）。
        # A field with an invalid name has its value left unchecked (010 of '1x' adds no error).
        text = (
            "constexprs:\n"
            "  Rate: {type: 'int // hz', value: 1}\n"
            "modules:\n"
            "  - module: Foo\n"
            "    id: foo\n"
            "    args:\n"
            "      - p: {'1x': 010, inner: {depth: 010}, items: [1, '2 /* two */']}\n"
            "    template_args: [int, '1 // one']\n"
        )
        comment = "C++ comments are not allowed inside a value; use a YAML # comment"
        with self.assertRaisesMessage(
            ConfigError,
            f"cfg.yaml: constexprs.Rate.type: {comment}\n"
            "cfg.yaml: foo.args.p: invalid field name 1x\n"
            "cfg.yaml: foo.args.p.inner.depth: 010 has a leading zero, which C++ reads as octal; "
            "write the decimal value\n"
            f"cfg.yaml: foo.args.p.items[1]: {comment}\n"
            f"cfg.yaml: foo.template_args[1]: {comment}",
        ):
            load(text)

    def test_monitor_sleep_is_a_decimal_u32(self):
        for value in ("0", "1", "1000", "4294967295"):
            with self.subTest(value=value):
                load(f"settings:\n  monitor_sleep_ms: {value}\n")
        for value in ("4294967296", "-1", "010", "0x10", "1e3", "1.5", "'10ms'"):
            with (
                self.subTest(value=value),
                self.assertRaisesMessage(
                    ConfigError,
                    f"cfg.yaml: settings.monitor_sleep_ms: {value.strip(chr(39))} is not an "
                    "unsigned 32-bit decimal millisecond count",
                ),
            ):
                load(f"settings:\n  monitor_sleep_ms: {value}\n")
        with self.assertRaisesMessage(
            ConfigError,
            "cfg.yaml: settings.monitor_sleep_ms must be an unsigned 32-bit decimal millisecond "
            "count",
        ):
            load("settings:\n  monitor_sleep_ms: null\n")

    def test_settings_accept_only_monitor_sleep(self):
        cases = [
            (
                "settings:\n  stack: 1024\n  heap: 1\n",
                "cfg.yaml: settings: unknown key(s) stack, heap; "
                "the only setting is monitor_sleep_ms",
            ),
            ("settings: [1]\n", "cfg.yaml: settings must be a mapping"),
            (
                'settings:\n  monitor_sleep_ms: "10"\n',
                'cfg.yaml: settings.monitor_sleep_ms: "10" is not an unsigned 32-bit decimal '
                "millisecond count",
            ),
        ]
        for text, message in cases:
            with self.subTest(text=text), self.assertRaises(ConfigError) as context:
                load(text)
            self.assertEqual(str(context.exception), message)

    def test_constexpr_includes_accept_quoted_and_system_header_names(self):
        load("constexpr_includes: [Foo.hpp, 'sub/Bar.h', '<vector>', ' <array> ']\n")
        for header, problem in (
            ("'\"Foo.hpp\"'", ': "Foo.hpp" is not'),
            ("'a b.hpp'", ": a b.hpp is not"),
            ("'<a\"b>'", ': <a"b> is not'),
            ("{a: 1}", " must be"),
        ):
            with (
                self.subTest(header=header),
                self.assertRaisesMessage(
                    ConfigError,
                    f"cfg.yaml: constexpr_includes[0]{problem} a header name such as Foo.hpp or "
                    "<vector>",
                ),
            ):
                load(f"constexpr_includes: [{header}]\n")

    def test_constexpr_names_namespace_and_shape_are_checked(self):
        load("constexpr_namespace: Board::Pins\nconstexprs:\n  Rate: {type: int, value: 250}\n")
        cases = [
            ("constexpr_namespace: std\n", "std is a reserved namespace"),
            ("constexpr_namespace: '1a'\n", "constexpr_namespace: 1a is not a C\\+\\+ namespace"),
            (
                'constexpr_namespace: "Board"\n',
                'constexpr_namespace: "Board" is not a C\\+\\+ namespace',
            ),
            ("constexpr_namespace: [a]\n", "constexpr_namespace must be a C\\+\\+ namespace"),
            ("constexpr_namespace: Board::class\n", "class is a C\\+\\+ keyword"),
            (
                "constexprs:\n  xr_rate: {type: int, value: 1}\n",
                "constexprs.xr_rate uses a prefix reserved",
            ),
            (
                "constexprs:\n  Rate: {type: int}\n",
                "constexprs.Rate requires exactly type and value",
            ),
            (
                "constexprs:\n  Rate: {type: int, value: 1, doc: x}\n",
                "constexprs.Rate requires exactly type and value",
            ),
            ("constexprs:\n  Rate: {type: int, value: 010}\n", "leading zero"),
        ]
        for text, pattern in cases:
            with self.subTest(text=text), self.assertRaisesRegex(ConfigError, pattern):
                load(text)

    def test_every_error_is_reported_once_with_the_config_prefix(self):
        text = (
            "extra: 1\nmodules:\n  - {module: Foo, id: class}\n  - {module: Foo, id: f, x: 1}\n"
            "  - {module: Foo, id: f}\nsettings: {monitor_sleep_ms: -1}\n"
        )
        with self.assertRaises(ConfigError) as context:
            load(text, "User/robot.yaml")
        lines = str(context.exception).splitlines()
        self.assertEqual(len(lines), 5, lines)
        self.assertTrue(all(line.startswith("User/robot.yaml: ") for line in lines), lines)


class LoadConfig(TempDirTestCase):
    """读取配置文件：编码和报错中的名字。
    Reading a configuration file: encoding and the name in errors.
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp

    def test_utf8_bom_is_accepted(self):
        path = self.write("cfg.yaml", "﻿modules: []\n")
        self.assertEqual(load_config(path), {"modules": []})

    def test_non_utf8_text_is_rejected(self):
        path = self.root / "cfg.yaml"
        path.write_bytes(b"modules: []\n# \xff\xfe\n")
        with self.assertRaisesMessage(ConfigError, "cfg.yaml: not UTF-8 text"):
            load_config(path, "cfg.yaml")

    def test_errors_use_the_given_source_name(self):
        path = self.write("cfg.yaml", "modules: {}\n")
        with self.assertRaisesMessage(
            ConfigError, "User/cfg.yaml: modules must be an ordered list"
        ):
            load_config(path, "User/cfg.yaml")


if __name__ == "__main__":
    unittest.main()
