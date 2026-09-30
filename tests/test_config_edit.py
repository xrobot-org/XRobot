"""编辑配置和 modules.yaml（xrobot.config_edit）：添加、修改、删除、重命名、格式化和同步。
Editing configurations and modules.yaml (xrobot.config_edit): add, set, remove, rename, format
and sync.
"""

import hashlib

from fixtures import BspTestCase, TempDirTestCase

from xrobot import config_edit
from xrobot.config import ConfigError, load_config, parse_yaml
from xrobot.generate_main import load_modules
from xrobot.type_index import TypeIndex

CONFIG = """# robot config
modules:
  # the status led
  - module: team/Led  # inline
    id: status
    args:
      - gpio: pin  # board pin
      - param: {cycle: 1, inverted: false, timing: {on_ms: 1, off_ms: 2}, name: "a"}
      - alt: Led::Defaults()
      - factory: Led::Defaults()
      - gain: 1.0f
  # the user of the led
  - module: team/User
    id: user
    args:
      - led: status  # bound
      - backup: '&status'
      - count: 3
# trailing comment
"""
USER = '#include "Led.hpp"\nclass User { public: User(Led& led, Led* backup, int count = 1) {} };'
LED = """namespace LibXR { class GPIO; }
struct Timing { int on_ms = 100; int off_ms{200}; };
class Led {
 public:
  struct Param { int cycle = 250; bool inverted{}; Timing timing; const char* name = "led"; };
  static Param Defaults() { return {}; }
  Led(LibXR::GPIO& gpio, Param param = {}, Param alt = {.cycle = 5}, Param factory = Defaults(), float gain = 1.0f) {}
};"""


class EditTestCase(BspTestCase):
    """带几个模块和一份配置的 BSP，供编辑测试使用。
    A BSP with a few Modules and one configuration for the editing tests.
    """

    def setUp(self):
        super().setUp()
        self.module("Led", LED)
        self.module("User", USER)
        self.path = self.config(CONFIG)

    def modules_and_index(self):
        """BSP 的模块和类型索引。
        The Modules and type index of the BSP.
        """
        modules = load_modules(self.project)
        return modules, TypeIndex.for_modules(modules)

    def add(self, module="Led", identity=None):
        """向配置添加一个实例，返回它的 id。
        Add an instance to the configuration and return its id.
        """
        modules, index = self.modules_and_index()
        return config_edit.add_instance(
            self.path, module, modules, index, identity, "User/xrobot.yaml"
        )

    def text(self):
        """配置的当前文本。
        The current configuration text.
        """
        return self.path.read_bytes().decode("utf-8")

    def instances(self):
        """配置中的实例。
        The instances of the configuration.
        """
        return load_config(self.path)["modules"]

    def assertUnchangedOnError(self, pattern, action, *args, **kwargs):
        """断言操作报错并且配置文件一个字节也没变。
        Assert that the action fails and leaves the configuration file byte for byte.
        """
        before = self.path.read_bytes()
        with self.assertRaisesRegex(ConfigError, pattern):
            action(*args, **kwargs)
        self.assertEqual(self.path.read_bytes(), before)


class AddInstance(EditTestCase):
    """instance add：按源码默认值添加实例。
    instance add: adding an instance with the source defaults.
    """

    def test_seeded_defaults_are_qualified_and_positional_ones_stay_cpp_code(self):
        self.module(
            "Qux",
            "namespace qux { enum Mode { A, B }; inline constexpr int kN = 3;\n"
            "  struct Param { Mode mode = A; int n = kN; }; }\n"
            "class Qux { public:\n  explicit Qux(qux::Param param = {},"
            " LibXR::Vector gains = {1.0f, 2.0f},"
            " Vendor::Options options = {.gains = {1, 2}, .on = true}) {} };",
        )
        self.add("Qux", "qux")
        self.assertEqual(
            self.instances()[-1]["args"],
            [
                {"param": {"mode": "qux::A", "n": "qux::kN"}},
                {"gains": "{1.0f, 2.0f}"},
                {"options": {"gains": "{1, 2}", "on": "true"}},
            ],
        )
        self.assertIn("      - gains: '{1.0f, 2.0f}'\n", self.text())

    def test_new_instances_are_seeded_from_source_defaults(self):
        self.assertEqual(self.add(), "led_0")
        added = self.instances()[-1]
        self.assertEqual((added["module"], added["id"]), ("team/Led", "led_0"))
        self.assertEqual(
            added["args"],
            [
                {"gpio": None},
                {
                    "param": {
                        "cycle": "250",
                        "inverted": "{}",
                        "timing": {"on_ms": "100", "off_ms": "{200}"},
                        "name": '"led"',
                    }
                },
                {
                    "alt": {
                        "cycle": "5",
                        "inverted": "{}",
                        "timing": {"on_ms": "100", "off_ms": "{200}"},
                        "name": '"led"',
                    }
                },
                {"factory": "Led::Defaults()"},
                {"gain": "1.0f"},
            ],
        )

    def test_comments_and_other_instances_are_kept(self):
        before = self.text()
        self.add(identity="second")
        after = self.text()
        self.assertTrue(after.startswith(before[: before.index("# trailing comment")]))
        for comment in (
            "# robot config",
            "# the status led",
            "# inline",
            "# board pin",
            "# the user of the led",
            "# bound",
            "# trailing comment",
        ):
            self.assertIn(comment, after)
        self.assertTrue(after.rstrip().endswith("# trailing comment"))
        self.assertEqual([i["id"] for i in self.instances()], ["status", "user", "second"])

    def test_generated_ids_skip_used_ones(self):
        self.assertEqual(config_edit.next_instance_id([], "DR16"), "dr16_0")
        self.assertEqual(
            config_edit.next_instance_id(
                [{"id": "dr160"}, {"id": "dr16_0"}, {"id": "dr16_2"}], "DR16"
            ),
            "dr16_1",
        )
        self.assertEqual(self.add(), "led_0")
        self.assertEqual(self.add(), "led_1")

    def test_invalid_or_non_standalone_additions_leave_the_file_alone(self):
        self.module(
            "Lib",
            "class Lib { public: Lib() {} };",
            manifest="/* === MODULE MANIFEST V2 ===\nstandalone: false\n=== END MANIFEST === */\n",
        )
        self.assertUnchangedOnError(
            "instance id class is a C\\+\\+ keyword", self.add, identity="class"
        )
        self.assertUnchangedOnError(
            r"team/Lib is a library \(standalone: false\) and cannot be instantiated",
            self.add,
            module="Lib",
        )
        with self.assertRaisesMessage(ValueError, "Module not found: Missing"):
            self.add(module="Missing")

    def test_adding_to_an_empty_or_missing_configuration(self):
        for text in ("modules: []\n", "settings:\n  monitor_sleep_ms: 10\n", None):
            with self.subTest(text=text):
                if text is None:
                    self.path.unlink()
                else:
                    self.path.write_text(text, encoding="utf-8")
                self.add(identity="led")
                self.assertEqual([i["id"] for i in self.instances()], ["led"])
                self.assertEqual(config_edit.format_files([self.path], check=True), [])

    def test_template_defaults_are_seeded(self):
        self.module(
            "Buf",
            "template <typename T = float, unsigned N = 4>\nclass Buf { public: explicit Buf(T init = T{}) {} };",
        )
        self.module(
            "Raw", "template <typename T>\nclass Raw { public: explicit Raw(T init = T{}) {} };"
        )
        self.add("Buf", "buf")
        self.add("Raw", "raw")
        buf, raw = self.instances()[-2:]
        self.assertEqual(buf["template_args"], ["float", "4"])
        self.assertEqual(buf["args"], [{"init": "float{}"}])
        self.assertEqual(raw["template_args"], [None])
        self.assertNotIn("args", raw)

    def test_the_result_is_in_the_canonical_layout(self):
        self.add()
        self.assertEqual(config_edit.format_files([self.path], check=True), [])


class SetValue(EditTestCase):
    """instance set：替换一个值，其余文本不变。
    instance set: replacing one value while the rest of the text stays.
    """

    def test_only_the_edited_line_changes(self):
        before = self.text().split("\n")
        config_edit.set_value(self.path, "user", "args.count", 5)
        after = self.text().split("\n")
        changed = [(a, b) for a, b in zip(before, after, strict=False) if a != b]
        self.assertEqual(changed, [("      - count: 3", "      - count: 5")])
        self.assertEqual(len(before), len(after))
        self.assertEqual(
            self.instances()[1]["args"], [{"led": "status"}, {"backup": "&status"}, {"count": "5"}]
        )

    def test_the_whole_argument_list_can_be_replaced(self):
        config_edit.set_value(self.path, "user", "args", [{"led": "status"}, {"count": 9}])
        user = load_config(self.path)["modules"][1]
        self.assertEqual(user["args"], [{"led": "status"}, {"count": "9"}])

    def test_paths_reach_arguments_fields_and_template_arguments(self):
        config_edit.set_value(self.path, "status", "args.param.timing.on_ms", 7)
        config_edit.set_value(self.path, "status", "args.gain", 2.5)
        config_edit.set_value(self.path, "status", "args.param.inverted", True)
        config_edit.set_value(
            self.path,
            "status",
            "args.alt",
            {"cycle": 1, "inverted": False, "timing": {"on_ms": 1, "off_ms": 2}, "name": '"b"'},
        )
        status = self.instances()[0]
        self.assertEqual(status["args"][1]["param"]["timing"], {"on_ms": "7", "off_ms": "2"})
        self.assertEqual(status["args"][1]["param"]["inverted"], "true")
        self.assertEqual(status["args"][4], {"gain": "2.5"})
        self.assertEqual(
            status["args"][2]["alt"],
            {
                "cycle": "1",
                "inverted": "false",
                "timing": {"on_ms": "1", "off_ms": "2"},
                "name": '"b"',
            },
        )
        self.assertIn("# board pin", self.text())

    def test_template_arguments_can_be_set(self):
        self.path.write_text(
            "modules:\n  - module: Buf\n    id: buf\n    template_args: [int, 4]\n",
            encoding="utf-8",
        )
        config_edit.set_value(self.path, "buf", "template_args[1]", 8)
        self.assertEqual(self.instances()[0]["template_args"], ["int", "8"])

    def test_id_and_module_are_not_set_values(self):
        self.assertUnchangedOnError(
            "an instance id is changed with `xrobot instance rename`",
            config_edit.set_value,
            self.path,
            "status",
            "id",
            "led",
        )
        self.assertUnchangedOnError(
            "the Module of an instance cannot be changed; remove the instance and add",
            config_edit.set_value,
            self.path,
            "user",
            "module",
            "team/Led",
        )

    def test_comments_inside_an_instance_keep_their_column(self):
        text = CONFIG.replace(
            "      - param: {cycle: 1, inverted: false, timing: {on_ms: 1, off_ms: 2}, "
            'name: "a"}\n',
            "      - param:\n          # cycle comment\n          cycle: 1\n"
            "          inverted: false\n          timing: {on_ms: 1, off_ms: 2}\n"
            '          # name comment\n          name: "a"\n',
        )
        self.path.write_text(text, encoding="utf-8")
        config_edit.set_value(self.path, "status", "args.param.timing", {"on_ms": 3, "off_ms": 4})
        after = self.text()
        self.assertIn("          # cycle comment\n          cycle: 1\n", after)
        self.assertIn('          # name comment\n          name: "a"\n', after)
        config_edit.rename_instance(self.path, "status", "led")
        self.assertIn("          # cycle comment\n          cycle: 1\n", self.text())

    def test_if_match_protects_against_concurrent_edits(self):
        self.path.write_bytes(self.path.read_bytes().replace(b"\n", b"\r\n"))
        current = hashlib.sha256(self.text().replace("\r\n", "\n").encode("utf-8")).hexdigest()
        self.assertEqual(config_edit.file_hash(self.path), current)
        self.assertUnchangedOnError(
            "User/xrobot.yaml changed since it was read; reload and retry",
            config_edit.set_value,
            self.path,
            "user",
            "args.count",
            5,
            "0" * 64,
            "User/xrobot.yaml",
        )
        config_edit.set_value(self.path, "user", "args.count", 5, current)
        self.assertEqual(self.instances()[1]["args"][2], {"count": "5"})
        self.assertNotIn("\r\n", self.text())

    def test_invalid_paths_and_results_are_rejected(self):
        cases = [
            ("user", "args", "args takes a list of one-parameter mappings"),
            ("user", "args.missing", "no argument missing"),
            ("status", "args.param.missing", "no key missing"),
            ("status", "args[9]", "no argument"),
            ("user", "template_args[0]", "no key template_args"),
            ("user", "args..count", "invalid path"),
            ("nobody", "args.count", "no instance with id nobody"),
        ]
        for identity, path, pattern in cases:
            with self.subTest(path=path):
                self.assertUnchangedOnError(
                    pattern, config_edit.set_value, self.path, identity, path, 1
                )
        self.assertUnchangedOnError(
            "invalid path cycle", config_edit.set_value, self.path, "status", "cycle", 1
        )


class RemoveAndRename(EditTestCase):
    """删除和重命名实例，以及对它的引用。
    Removing and renaming instances, and the references to them.
    """

    def test_a_referenced_instance_cannot_be_removed(self):
        self.assertUnchangedOnError(
            "status is still used by user; change those values first",
            config_edit.remove_instance,
            self.path,
            "status",
            "User/xrobot.yaml",
        )

    def test_removal_takes_the_instance_comments_with_it(self):
        config_edit.remove_instance(self.path, "user")
        text = self.text()
        self.assertNotIn("# the user of the led", text)
        self.assertNotIn("id: user", text)
        for comment in ("# robot config", "# the status led", "# board pin", "# trailing comment"):
            self.assertIn(comment, text)

    def test_the_last_instance_can_be_removed(self):
        config_edit.remove_instance(self.path, "user")
        config_edit.remove_instance(self.path, "status")
        self.assertEqual(load_config(self.path)["modules"], [])
        self.assertIn("# trailing comment", self.text())

    def test_rename_updates_every_reference(self):
        self.path.write_text(
            CONFIG.replace(
                "      - count: 3",
                '      - count: status.Count() + ns::status + obj.status + sizeof("status")\n'
                '      - label: "status"',
            ),
            encoding="utf-8",
        )
        config_edit.rename_instance(self.path, "status", "led")
        user = self.instances()[1]
        self.assertEqual(self.instances()[0]["id"], "led")
        self.assertEqual(user["args"][0], {"led": "led"})
        self.assertEqual(user["args"][1], {"backup": "&led"})
        self.assertEqual(
            user["args"][2], {"count": 'led.Count() + ns::status + obj.status + sizeof("status")'}
        )
        self.assertEqual(user["args"][3], {"label": '"status"'})
        self.assertIn("      - backup: '&led'\n", self.text())
        self.assertIn('      - label: "status"\n', self.text())
        for comment in ("# the status led", "# bound", "# the user of the led"):
            self.assertIn(comment, self.text())

    def test_rename_rejects_invalid_and_existing_ids(self):
        self.assertUnchangedOnError(
            "instance id user already exists",
            config_edit.rename_instance,
            self.path,
            "status",
            "user",
            "User/xrobot.yaml",
        )
        self.assertUnchangedOnError(
            "ASSERT is a macro name", config_edit.rename_instance, self.path, "status", "ASSERT"
        )
        self.assertUnchangedOnError(
            "no instance with id nobody", config_edit.rename_instance, self.path, "nobody", "x"
        )


class Format(TempDirTestCase):
    """format：规范格式。
    format: the canonical layout.
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp

    def test_canonical_layout_indents_sequences_uses_lf_and_drops_the_bom(self):
        path = self.write(
            "a.yaml",
            "﻿# keep\r\nmodules:\r\n- module: Led   # inline\r\n  id: a\r\n  args:\r\n"
            "  - n: 1\r\n  - m: [1, 2]\r\n",
        )
        self.assertEqual(config_edit.format_files([path], check=True), [path])
        self.assertTrue(path.read_bytes().startswith(b"\xef\xbb\xbf"))
        self.assertEqual(config_edit.format_files([path]), [path])
        text = path.read_bytes().decode("utf-8")
        self.assertRegex(
            text,
            r"^# keep\nmodules:\n  - module: Led +# inline\n    id: a\n    args:\n      - n: 1\n"
            r"      - m: \[1, 2\]\n$",
        )
        self.assertEqual(config_edit.format_files([path]), [])
        self.assertEqual(path.read_bytes().decode("utf-8"), text)

    def test_code_is_written_without_quotes_where_yaml_allows_and_strings_in_double_quotes(self):
        text = (
            "modules:\n  - module: Led\n    id: a\n    args:\n"
            "      - name: '\"x\"'\n"
            "      - cycle: '250'\n"
            "      - rotation: '{0.707, 0.0}'\n"
            "      - ref: '&a'\n"
            "      - p: {a: '1', b: '\"2\"', c: '{}', d: 'LibXR::Terminal<32, 32>'}\n"
            "      - q:\n          e: 'LibXR::Terminal<32, 32>'\n          f: ['\"g\"', 'h']\n"
            '      - text: "say \\"hi\\" 中文"\n'
            '      - raw: \'"a" "b"\'\n'
        )
        expected = (
            "modules:\n  - module: Led\n    id: a\n    args:\n"
            '      - name: "x"\n'
            "      - cycle: 250\n"
            "      - rotation: '{0.707, 0.0}'\n"
            "      - ref: '&a'\n"
            "      - p: {a: 1, b: \"2\", c: '{}', d: 'LibXR::Terminal<32, 32>'}\n"
            '      - q:\n          e: LibXR::Terminal<32, 32>\n          f: ["g", h]\n'
            '      - text: "say \\"hi\\" 中文"\n'
            '      - raw: \'"a" "b"\'\n'
        )
        self.assertEqual(config_edit.canonical_text(text), expected)
        self.assertEqual(config_edit.canonical_text(expected), expected)
        self.assertEqual(parse_yaml(expected, "b.yaml"), parse_yaml(text, "b.yaml"))


class SyncConfig(EditTestCase):
    """sync：跟随模块接口的变化更新配置。
    sync: updating configurations to the Module interfaces.
    """

    def test_new_fields_and_parameters_are_added_and_removed_fields_dropped(self):
        self.module(
            "Led",
            LED.replace("bool inverted{};", "int extra = 7;").replace(
                "float gain = 1.0f) {}", "float gain = 1.0f, int scale = 2) {}"
            ),
        )
        modules, index = self.modules_and_index()
        diff = config_edit.sync_config(self.path, modules, index, "User/xrobot.yaml")
        self.assertTrue(diff.startswith("--- User/xrobot.yaml\n+++ User/xrobot.yaml\n"), diff)
        status = self.instances()[0]
        self.assertEqual(list(status["args"][1]["param"]), ["cycle", "extra", "timing", "name"])
        self.assertEqual(status["args"][1]["param"]["extra"], "7")
        self.assertEqual(status["args"][1]["param"]["cycle"], "1")
        self.assertEqual(status["args"][5], {"scale": "2"})
        for comment in (
            "# robot config",
            "# inline",
            "# board pin",
            "# bound",
            "# trailing comment",
        ):
            self.assertIn(comment, self.text())
        self.assertEqual(config_edit.sync_config(self.path, modules, index, "User/xrobot.yaml"), "")

    def test_nested_fields_are_synced_and_order_restored(self):
        self.module(
            "Led",
            LED.replace(
                "struct Timing { int on_ms = 100; int off_ms{200}; };",
                "struct Timing { int off_ms{200}; int on_ms = 100; int period = 9; };",
            ),
        )
        modules, index = self.modules_and_index()
        config_edit.sync_config(self.path, modules, index)
        self.assertEqual(
            self.instances()[0]["args"][1]["param"]["timing"],
            {"off_ms": "2", "on_ms": "1", "period": "9"},
        )
        self.assertEqual(
            list(self.instances()[0]["args"][1]["param"]["timing"]), ["off_ms", "on_ms", "period"]
        )

    def test_constructor_keyed_mappings_follow_a_changed_constructor(self):
        self.module(
            "Clock",
            "class Clock { public:\n"
            "  class Runtime { public: Runtime(int period, int legacy_div = 3) {} };\n"
            "  explicit Clock(Runtime runtime = Runtime(1)) {} };",
        )
        self.config(
            "modules:\n  - module: team/Clock\n    id: clock\n    args:\n"
            "      - runtime: {period: 5, legacy_div: 2}\n"
        )
        self.module(
            "Clock",
            "class Clock { public:\n"
            "  class Runtime { public: Runtime(int period, unsigned settle_us = 10U) {} };\n"
            "  explicit Clock(Runtime runtime = Runtime(1)) {} };",
        )
        modules, index = self.modules_and_index()
        config_edit.sync_config(self.path, modules, index)
        self.assertEqual(
            self.instances()[0]["args"][0]["runtime"], {"period": "5", "settle_us": "10U"}
        )

    def test_instances_that_match_no_constructor_are_left_alone(self):
        self.module("User", USER.replace("int count = 1", "Led* spare, int count = 1"))
        modules, index = self.modules_and_index()
        before = self.text()
        self.assertEqual(config_edit.sync_config(self.path, modules, index), "")
        self.assertEqual(self.text(), before)


class ModulesYaml(TempDirTestCase):
    """module add/remove 对 modules.yaml 的编辑。
    module add/remove editing modules.yaml.
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp
        self.path = self.write(
            "modules.yaml", "# pins\nxrobot: 1.0.0\nmodules:\n  - team/A@dev  # keep\n"
        )

    def test_add_defaults_to_same_or_dev_and_keeps_explicit_refs(self):
        config_edit.add_module(self.path, "team/B")
        config_edit.add_module(self.path, "team/C@v1")
        text = self.path.read_text(encoding="utf-8")
        self.assertIn("  - team/B@same-or-dev\n", text)
        self.assertIn("  - team/C@v1\n", text)
        for kept in ("# pins", "xrobot: 1.0.0", "team/A@dev  # keep"):
            self.assertIn(kept, text)

    def test_duplicates_and_non_canonical_ids_are_rejected(self):
        with self.assertRaisesMessage(ConfigError, f"team/a is already requested in {self.path}"):
            config_edit.add_module(self.path, "team/a@master")
        with self.assertRaisesMessage(ValueError, "Expected canonical owner/repo: 'A'"):
            config_edit.add_module(self.path, "A")

    def test_remove(self):
        config_edit.remove_module(self.path, "TEAM/A")
        self.assertNotIn("team/A", self.path.read_text(encoding="utf-8"))
        self.assertIn("xrobot: 1.0.0", self.path.read_text(encoding="utf-8"))
        with self.assertRaisesMessage(ConfigError, f"team/A is not requested in {self.path}"):
            config_edit.remove_module(self.path, "team/A")

    def test_a_missing_modules_yaml_is_created(self):
        path = self.root / "new/modules.yaml"
        config_edit.add_module(path, "team/A")
        self.assertEqual(path.read_text(encoding="utf-8"), "modules:\n  - team/A@same-or-dev\n")
