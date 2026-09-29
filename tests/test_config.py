"""Loading and validating application configurations (xrobot.config)."""

import unittest

from fixtures import TempDirTestCase

from xrobot.config import (
    ConfigError,
    identifier_problem,
    load_config,
    parse_yaml,
    validate_config,
    value_text,
)


def load(text, source="cfg.yaml"):
    config = parse_yaml(text, source)
    validate_config(config, source)
    return config


def instance(**values):
    return "modules:\n  - module: Foo\n    id: foo\n    args:\n" + "".join(
        "      - {}: {}\n".format(*item) for item in values.items()
    )


class Scalars(unittest.TestCase):
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

    def test_null_tilde_and_empty_mean_not_filled_but_quoted_null_is_text(self):
        config = load(instance(a="null", b="~", c="", d="'null'", e="nullptr", f="NULL"))
        self.assertEqual(
            config["modules"][0]["args"],
            [{"a": None}, {"b": None}, {"c": None}, {"d": "null"}, {"e": "nullptr"}, {"f": None}],
        )

    def test_unfilled_values_are_a_valid_saved_configuration(self):
        load(instance(a="null"))

    def test_unfilled_value_is_rejected_when_its_text_is_needed(self):
        with self.assertRaisesRegex(ConfigError, r"foo\.args\.a is not filled in .*write nullptr"):
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


class YamlFeatures(unittest.TestCase):
    def test_anchors_and_aliases_are_rejected_with_a_hint(self):
        for text in ("a: &x 1\nb: *x\n", "modules:\n  - &first {module: Foo, id: foo}\n"):
            with (
                self.subTest(text=text),
                self.assertRaisesRegex(
                    ConfigError,
                    r"cfg\.yaml:\d+: YAML anchors and aliases are not allowed.*constexprs",
                ),
            ):
                load(text)

    def test_tags_are_rejected(self):
        for text in (
            "modules: !!seq []\n",
            "settings:\n  monitor_sleep_ms: !custom 10\n",
            "constexprs: {a: {type: int, value: !!str '1'}}\n",
        ):
            with (
                self.subTest(text=text),
                self.assertRaisesRegex(ConfigError, r"cfg\.yaml:\d+: YAML tags"),
            ):
                load(text)

    def test_duplicate_keys_are_rejected_with_their_line(self):
        with self.assertRaisesRegex(ConfigError, r"cfg\.yaml:2: duplicate key modules"):
            load("modules: []\nmodules: []\n")
        with self.assertRaisesRegex(ConfigError, r"cfg\.yaml:4: duplicate key id"):
            load("modules:\n  - module: Foo\n    id: a\n    id: b\n")

    def test_complex_mapping_keys_are_rejected(self):
        with self.assertRaisesRegex(ConfigError, "mapping keys must be plain names"):
            load("{a: 1}: 2\n")

    def test_syntax_errors_carry_file_and_line(self):
        with self.assertRaisesRegex(ConfigError, r"cfg\.yaml:3: YAML syntax error"):
            load("modules:\n  - module: Foo\n  id: [\n")


class CppText(unittest.TestCase):
    def test_cpp_comments_inside_a_value_are_rejected(self):
        for value in ("1 // tuned", "'1 /* tuned */'"):
            with (
                self.subTest(value=value),
                self.assertRaisesRegex(
                    ConfigError,
                    r"cfg\.yaml: foo\.args\.a: C\+\+ comments are not allowed.*YAML # comment",
                ),
            ):
                load(instance(a=value))

    def test_comment_markers_inside_string_literals_are_text(self):
        load(instance(a="'\"http://x\"'", b="'\"/*\"'"))

    def test_leading_zero_integers_are_rejected_as_octal(self):
        for value in ("010", "00", "'0010'", "017u", "'f(07)'"):
            with (
                self.subTest(value=value),
                self.assertRaisesRegex(ConfigError, "leading zero.*octal"),
            ):
                load(instance(a=value))

    def test_zero_hex_and_decimal_fractions_are_accepted(self):
        load(instance(a="0", b="0x10", c="0.5", d="0b101", e="1e-3", f="10"))

    def test_non_ascii_digits_are_rejected(self):
        for value in ("１２", "'f(٣)'"):
            with self.subTest(value=value), self.assertRaisesRegex(ConfigError, "non-ASCII digits"):
                load(instance(a=value))

    def test_at_syntax_is_rejected(self):
        with self.assertRaisesRegex(ConfigError, "not @ syntax"):
            load(instance(a="'@nullptr'"))


class Identifiers(unittest.TestCase):
    def test_valid_identifiers(self):
        for name in ("led", "motor_1", "_private", "Chassis2", "xrobotic", "XRay"):
            with self.subTest(name=name):
                self.assertIsNone(identifier_problem(name))

    def test_invalid_identifiers(self):
        cases = {
            "1led": "not a C\\+\\+ identifier",
            "a-b": "not a C\\+\\+ identifier",
            "": "not a C\\+\\+ identifier",
            "class": "keyword",
            "and": "keyword",
            "final": "keyword",
            "co_await": "keyword",
            "ASSERT": "macro",
            "NULL": "macro",
            "XR_REGISTER": "macro",
            "assert": "macro",
            "std": "reserved namespace",
            "LibXR": "reserved namespace",
            "xrobot_generated": "reserved namespace",
            "xr_led": "prefix reserved",
            "XR_LED": "prefix reserved",
            "xrobot_led": "prefix reserved",
            "__led": "reserved by the C\\+\\+ standard",
            "_Led": "reserved by the C\\+\\+ standard",
        }
        for name, reason in cases.items():
            with self.subTest(name=name):
                self.assertRegex(identifier_problem(name) or "", reason)

    def test_invalid_instance_ids_are_rejected(self):
        for identity in ("CMD2 x", "ASSERT", "while", "xr_led"):
            with (
                self.subTest(identity=identity),
                self.assertRaisesRegex(ConfigError, r"modules\[0\]\.id"),
            ):
                load(f"modules:\n  - module: Foo\n    id: {identity}\n")


class Structure(unittest.TestCase):
    def test_unknown_top_level_keys_are_rejected(self):
        for key in ("global_settings", "instances", "constexpr"):
            with (
                self.subTest(key=key),
                self.assertRaisesRegex(ConfigError, "unknown top-level key"),
            ):
                load(f"{key}: {{}}\n")

    def test_unknown_instance_keys_are_rejected(self):
        for key in ("name", "constructor_args", "depends"):
            with (
                self.subTest(key=key),
                self.assertRaisesRegex(ConfigError, r"modules\[0\]: unknown key\(s\) " + key),
            ):
                load(f"modules:\n  - module: Foo\n    id: foo\n    {key}: []\n")

    def test_instances_need_module_and_id(self):
        with self.assertRaisesRegex(ConfigError, r"modules\[0\]\.module is required"):
            load("modules:\n  - id: foo\n")
        with self.assertRaisesRegex(ConfigError, r"modules\[0\]\.id is required"):
            load("modules:\n  - module: Foo\n")

    def test_duplicate_instance_ids_are_rejected(self):
        with self.assertRaisesRegex(ConfigError, r"modules\[1\]\.id: duplicate instance id foo"):
            load("modules:\n  - {module: Foo, id: foo}\n  - {module: Foo, id: foo}\n")

    def test_arguments_are_an_ordered_list_of_single_named_values(self):
        cases = [
            ("args: {a: 1}", "args must be an ordered list"),
            ("args: [{a: 1, b: 2}]", r"args\[0\] requires one named parameter"),
            ("args: [1]", r"args\[0\] requires one named parameter"),
            ("args: [{a: 1}, {a: 2}]", "invalid or duplicate parameter name a"),
            ("template_args: int", "template_args must be an ordered list"),
        ]
        for text, pattern in cases:
            with self.subTest(text=text), self.assertRaisesRegex(ConfigError, pattern):
                load(f"modules:\n  - module: Foo\n    id: foo\n    {text}\n")

    def test_nested_field_names_must_be_identifiers(self):
        with self.assertRaisesRegex(ConfigError, "invalid field name 1x"):
            load("modules:\n  - module: Foo\n    id: foo\n    args:\n      - p: {'1x': 1}\n")

    def test_monitor_sleep_is_a_decimal_u32(self):
        for value in ("0", "1", "1000", "4294967295"):
            with self.subTest(value=value):
                load(f"settings:\n  monitor_sleep_ms: {value}\n")
        for value in ("4294967296", "-1", "010", "0x10", "1e3", "1.5", "'10ms'", "null"):
            with self.subTest(value=value), self.assertRaisesRegex(ConfigError, "monitor_sleep_ms"):
                load(f"settings:\n  monitor_sleep_ms: {value}\n")

    def test_settings_accept_only_monitor_sleep(self):
        with self.assertRaisesRegex(ConfigError, "settings only accepts monitor_sleep_ms"):
            load("settings:\n  stack: 1024\n")

    def test_constexpr_includes_accept_quoted_and_system_header_names(self):
        load("constexpr_includes: [Foo.hpp, 'sub/Bar.h', '<vector>', ' <array> ']\n")
        for header in ("'\"Foo.hpp\"'", "'a b.hpp'", "'<a\"b>'", "{a: 1}"):
            with (
                self.subTest(header=header),
                self.assertRaisesRegex(ConfigError, r"constexpr_includes\[0\]"),
            ):
                load(f"constexpr_includes: [{header}]\n")

    def test_constexpr_names_namespace_and_shape_are_checked(self):
        load("constexpr_namespace: Board::Pins\nconstexprs:\n  Rate: {type: int, value: 250}\n")
        cases = [
            ("constexpr_namespace: std\n", "std is a reserved namespace"),
            ("constexpr_namespace: '1a'\n", "constexpr_namespace must be a C\\+\\+ namespace name"),
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
            "extra: 1\nmodules:\n  - {module: Foo, id: class}\n  - {module: Foo, id: f, name: x}\n"
            "  - {module: Foo, id: f}\nsettings: {monitor_sleep_ms: -1}\n"
        )
        with self.assertRaises(ConfigError) as context:
            load(text, "User/robot.yaml")
        lines = str(context.exception).splitlines()
        self.assertEqual(len(lines), 5, lines)
        self.assertTrue(all(line.startswith("User/robot.yaml: ") for line in lines), lines)


class LoadConfig(TempDirTestCase):
    def setUp(self):
        super().setUp()
        self.root = self.tmp

    def test_utf8_bom_is_accepted(self):
        path = self.write("cfg.yaml", "﻿modules: []\n")
        self.assertEqual(load_config(path), {"modules": []})

    def test_non_utf8_text_is_rejected(self):
        path = self.root / "cfg.yaml"
        path.write_bytes(b"modules: []\n# \xff\xfe\n")
        with self.assertRaisesRegex(ConfigError, "not UTF-8"):
            load_config(path, "cfg.yaml")

    def test_errors_use_the_given_source_name(self):
        path = self.write("cfg.yaml", "modules: {}\n")
        with self.assertRaisesRegex(ConfigError, "^User/cfg.yaml: modules must be an ordered list"):
            load_config(path, "User/cfg.yaml")


if __name__ == "__main__":
    unittest.main()
