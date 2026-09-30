"""`describe`: the BSP state as JSON for editors (xrobot.describe)."""

import json
import os

from fixtures import BspTestCase

from xrobot import __version__
from xrobot.describe import SCHEMA, describe
from xrobot.generate_main import generate

MODULES = {
    "Led": "namespace LibXR { class GPIO; }\nclass Led { public:\n"
    "  struct Param { int cycle = 250; bool inverted; };\n"
    "  Led(LibXR::GPIO& gpio, const Param& param = {.cycle = 250, .inverted = false}) {}\n"
    "  void OnMonitor() {} };",
    "Motor": "class Motor { public: Motor() {} virtual ~Motor() = default; };",
    "Wheel": '#include "Motor.hpp"\nclass Wheel : public Motor { public:\n'
    "  struct Config { struct Pid { float k; } pid; int id = 1; };\n"
    "  class Limits { public: Limits(float max_speed, float max_current) {} };\n"
    "  explicit Wheel(Config config = {}, Limits limits = Limits(1.0F, 2.0F)) {} };",
    "Base": '#include "Motor.hpp"\nclass Base { public: explicit Base(Motor& motor, int rate = 1) {} };',
    "Chain": "class Chain { public: explicit Chain(Chain* previous) {} };",
}
MAIN = (
    '#include "xrobot_main.hpp"\nvoid app_main() {\n  XR_REGISTER(led_pin, LibXR::GPIO);\n'
    "  XR_REGISTER(uart1, LibXR::UART);\n  XR_REGISTER(motor0, Motor);\n  XROBOT_MAIN();\n}\n"
)
CONFIG = {
    "constexprs": {"Rate": {"type": "int", "value": "5"}},
    "modules": [
        {
            "module": "Led",
            "id": "led",
            "args": [{"gpio": "led_pin"}, {"param": {"cycle": "100", "inverted": "true"}}],
        },
        {"module": "Base", "id": "base0", "args": [{"motor": "motor0"}, {"rate": "1"}]},
        {
            "module": "Wheel",
            "id": "wheel",
            "args": [
                {"config": {"pid": {"k": "1.0F"}, "id": "1"}},
                {"limits": {"max_speed": "3.0F", "max_current": "4.0F"}},
            ],
        },
        {
            "module": "Base",
            "id": "base",
            "args": [{"motor": "wheel"}, {"rate": "ProjectConstexpr::Rate"}],
        },
        {"module": "Chain", "id": "first", "args": [{"previous": "nullptr"}]},
        {"module": "Chain", "id": "second", "args": [{"previous": "&first"}]},
    ],
}


class DescribeTestCase(BspTestCase):
    def setUp(self):
        super().setUp()
        for name, text in MODULES.items():
            self.module(name, text)
        self.entry(MAIN)
        self.path = self.config(CONFIG)

    def messages(self, result, severity=None):
        return [d["message"] for d in result["diagnostics"] if severity in (None, d["severity"])]


class Shape(DescribeTestCase):
    def test_top_level_fields(self):
        result = describe(self.project)
        self.assertEqual(SCHEMA, 1)
        self.assertEqual(
            set(result),
            {
                "schema",
                "root",
                "configs",
                "config",
                "selected",
                "header",
                "tools",
                "lock",
                "entry",
                "registrations",
                "modules",
                "types",
                "instances",
                "constexprs",
                "diagnostics",
            },
        )
        self.assertEqual(result["schema"], 1)
        self.assertEqual(result["root"], self.root.as_posix())
        self.assertEqual(
            (result["configs"], result["config"], result["selected"]),
            (["User/xrobot.yaml"], "User/xrobot.yaml", "User/xrobot.yaml"),
        )
        self.assertEqual(result["entry"], "User/app_main.cpp")
        self.assertEqual(
            result["tools"], {"xrobot": {"installed": __version__, "pin": __version__}}
        )
        self.assertEqual(json.loads(json.dumps(result)), result)

    def test_a_clean_bsp_only_warns_about_the_missing_header(self):
        result = describe(self.project)
        self.assertEqual(
            result["diagnostics"],
            [
                {
                    "severity": "warning",
                    "scope": "User/xrobot_main.hpp",
                    "message": "not generated; run `xrobot setup`",
                }
            ],
        )
        self.assertEqual(result["header"]["status"], "missing")

    def test_describe_never_writes(self):
        before = {
            p: p.read_bytes() for p in self.root.rglob("*") if p.is_file() and ".git" not in p.parts
        }
        describe(self.project)
        after = {
            p: p.read_bytes() for p in self.root.rglob("*") if p.is_file() and ".git" not in p.parts
        }
        self.assertEqual(before, after)

    def test_registrations_lock_and_constexprs(self):
        result = describe(self.project)
        self.assertEqual(
            result["registrations"],
            [
                {"name": "led_pin", "type": "LibXR::GPIO"},
                {"name": "uart1", "type": "LibXR::UART"},
                {"name": "motor0", "type": "Motor"},
            ],
        )
        self.assertEqual(result["lock"]["status"], "ok")
        self.assertEqual(
            [m["id"] for m in result["lock"]["modules"]], sorted("team/" + n for n in MODULES)
        )
        self.assertTrue(
            all(m["commit"] == m["head"] and m["status"] == "ok" for m in result["lock"]["modules"])
        )
        self.assertEqual(
            result["constexprs"],
            [{"name": "Rate", "qualified": "ProjectConstexpr::Rate", "type": "int"}],
        )


class ModulesAndTypes(DescribeTestCase):
    def test_constructor_parameters(self):
        modules = describe(self.project)["modules"]
        self.assertEqual(set(modules), {"team/" + n for n in MODULES})
        led = modules["team/Led"]
        self.assertEqual(
            (led["class"], led["header"], led["standalone"]),
            ("Led", "Modules/team/Led/Led.hpp", True),
        )
        gpio, param = led["constructors"][0]["parameters"]
        self.assertEqual(
            gpio,
            {
                "name": "gpio",
                "type": "LibXR::GPIO&",
                "default": None,
                "dependency": True,
                "default_fields": None,
                "type_ref": None,
                "candidates": ["led_pin"],
            },
        )
        self.assertEqual(param["type"], "const Led::Param&")
        self.assertEqual(param["default_fields"], {"cycle": "250", "inverted": "false"})
        self.assertEqual((param["type_ref"], param["dependency"]), ("Led::Param", False))
        motor = modules["team/Base"]["constructors"][0]["parameters"][0]
        self.assertEqual(motor["candidates"], ["motor0"])

    def test_types_describe_mapping_shapes_with_field_defaults(self):
        types = describe(self.project)["types"]
        self.assertEqual(
            types["Led::Param"],
            {
                "kind": "aggregate",
                "fields": [
                    {"name": "cycle", "type": "int", "type_ref": None, "default": "250"},
                    {"name": "inverted", "type": "bool", "type_ref": None, "default": None},
                ],
            },
        )
        config = types["Wheel::Config"]
        self.assertEqual(
            [(f["name"], f["type_ref"], f["default"]) for f in config["fields"]],
            [("pid", "Wheel::Config::Pid", None), ("id", None, "1")],
        )
        self.assertEqual(types["Wheel::Config::Pid"]["fields"][0]["name"], "k")
        self.assertEqual(types["Wheel::Limits"]["kind"], "class")
        self.assertEqual(
            [[p["name"] for p in c] for c in types["Wheel::Limits"]["constructors"]],
            [["max_speed", "max_current"]],
        )

    def test_unmappable_types_are_opaque(self):
        self.module(
            "U",
            "class U { public: struct V { union { int a; float b; } u; }; explicit U(V v = {}) {} };",
        )
        types = describe(self.project)["types"]
        self.assertEqual(types["U::V"]["kind"], "opaque")
        self.assertIn("contains a union", types["U::V"]["reason"])

    def test_template_parameters_and_libraries(self):
        self.module(
            "Buf",
            "template <typename T, unsigned N = 4>\nclass Buf { public: explicit Buf(T init = T{}) {} };",
        )
        self.module(
            "Lib",
            "class Lib { public: Lib() {} };",
            manifest="/* === MODULE MANIFEST V2 ===\nstandalone: false\n=== END MANIFEST === */\n",
        )
        modules = describe(self.project)["modules"]
        self.assertEqual(
            modules["team/Buf"]["template_parameters"],
            [
                {"name": "T", "type": "typename", "default": None},
                {"name": "N", "type": "unsigned", "default": "4"},
            ],
        )
        self.assertEqual(
            modules["team/Lib"],
            {
                "id": "team/Lib",
                "class": "Lib",
                "header": "Modules/team/Lib/Lib.hpp",
                "standalone": False,
            },
        )


class Instances(DescribeTestCase):
    def test_instances_use_the_module_keys(self):
        result = describe(self.project)
        self.assertEqual(
            [i["id"] for i in result["instances"]],
            ["led", "base0", "wheel", "base", "first", "second"],
        )
        for instance in result["instances"]:
            self.assertIn(instance["module"], result["modules"])
        led = result["instances"][0]
        self.assertEqual(
            (led["module"], led["class"], led["template_args"]), ("team/Led", "Led", [])
        )
        self.assertEqual(led["args"][1], {"param": {"cycle": "100", "inverted": "true"}})

    def test_candidates_are_earlier_instances_registrations_and_constants(self):
        instances = {i["id"]: i for i in describe(self.project)["instances"]}
        self.assertEqual(instances["base0"]["candidates"]["motor"], ["motor0"])
        self.assertEqual(instances["base"]["candidates"]["motor"], ["motor0", "wheel"])
        self.assertEqual(instances["base"]["candidates"]["rate"], ["ProjectConstexpr::Rate"])
        self.assertEqual(instances["first"]["candidates"]["previous"], ["nullptr"])
        self.assertEqual(instances["second"]["candidates"]["previous"], ["&first", "nullptr"])
        self.assertEqual(instances["led"]["candidates"], {"gpio": ["led_pin"], "param": []})

    def test_unknown_modules_are_described_as_written(self):
        self.config({"modules": [{"module": "Missing", "id": "m"}]})
        result = describe(self.project)
        self.assertEqual(
            result["instances"],
            [
                {
                    "id": "m",
                    "module": "Missing",
                    "class": None,
                    "template_args": [],
                    "args": [],
                    "candidates": {},
                }
            ],
        )
        self.assertIn(
            "User/xrobot.yaml: m: Module not found: Missing", self.messages(result, "error")
        )


class Diagnostics(DescribeTestCase):
    def test_generation_errors_are_reported(self):
        self.config(
            {
                "modules": [
                    {
                        "module": "Led",
                        "id": "led",
                        "args": [{"gpio": "missing"}, {"param": {"cycle": "1"}}],
                    }
                ]
            }
        )
        messages = self.messages(describe(self.project), "error")
        self.assertTrue(any("led.args.gpio: missing is neither" in m for m in messages), messages)
        self.assertTrue(any("led.args.param: missing inverted" in m for m in messages), messages)

    def test_invalid_configuration_is_reported(self):
        self.config("modules:\n  - module: Led\n    id: class\n")
        messages = self.messages(describe(self.project), "error")
        self.assertTrue(
            any("modules[0].id: class is a C++ keyword" in m for m in messages), messages
        )

    def test_header_freshness(self):
        generate(self.project)
        result = describe(self.project)
        self.assertEqual(result["header"]["status"], "fresh")
        self.assertEqual(result["diagnostics"], [])
        future = (self.root / "User/xrobot_main.hpp").stat().st_mtime + 10
        os.utime(self.path, (future, future))
        result = describe(self.project)
        self.assertEqual(result["header"]["status"], "stale")
        self.assertIn(
            "generated from older inputs (User/xrobot.yaml); run `xrobot gen`",
            self.messages(result, "warning"),
        )

    def test_another_config_can_be_described_without_changing_the_selection(self):
        generate(self.project)
        alt = self.config({"modules": [{"module": "Motor", "id": "m"}]}, name="products/alt.yaml")
        result = describe(self.project, alt)
        self.assertEqual(
            (result["config"], result["selected"]), ("User/products/alt.yaml", "User/xrobot.yaml")
        )
        self.assertEqual([i["id"] for i in result["instances"]], ["m"])
        self.assertEqual(result["configs"], ["User/products/alt.yaml", "User/xrobot.yaml"])

    def test_a_selected_config_that_no_longer_exists_is_a_diagnostic(self):
        alt = self.config({"modules": [{"module": "Motor", "id": "m"}]}, name="products/alt.yaml")
        generate(self.project, alt)
        alt.unlink()
        result = describe(self.project)
        self.assertEqual(result["selected"], "User/products/alt.yaml")
        self.assertEqual(result["configs"], ["User/xrobot.yaml"])
        self.assertIn(
            "User/xrobot_main.hpp was generated for User/products/alt.yaml, which does not exist; "
            "select a configuration with `xrobot gen -c <config>`",
            self.messages(result, "error"),
        )

    def test_tool_pin_warnings(self):
        self.write("Modules/modules.yaml", "modules: []\n")
        result = describe(self.project)
        self.assertEqual(result["tools"]["xrobot"]["pin"], None)
        self.assertIn(
            f"XRobot is not pinned; add `xrobot: {__version__}`", self.messages(result, "warning")
        )
        self.write("Modules/modules.yaml", "xrobot: 0.9.0\nmodules: []\n")
        self.assertIn(
            f"installed XRobot {__version__} differs from the pinned 0.9.0",
            self.messages(describe(self.project), "warning"),
        )
        self.write("Modules/modules.yaml", "xrobot: latest\nmodules: []\n")
        self.assertTrue(
            any(
                "xrobot must be a release version" in m
                for m in self.messages(describe(self.project), "error")
            )
        )

    def test_lock_problems_are_reported_without_reading_those_modules(self):
        self.locked["team/Gone"] = "1" * 40
        self.locked["team/Led"] = "0" * 40
        self.write_lock()
        result = describe(self.project)
        self.assertEqual(
            {m["id"]: m["status"] for m in result["lock"]["modules"]}["team/Gone"], "missing"
        )
        self.assertEqual(
            {m["id"]: m["status"] for m in result["lock"]["modules"]}["team/Led"], "mismatch"
        )
        self.assertIn(result["lock"]["status"], ("missing", "mismatch"))
        self.assertNotIn("team/Gone", result["modules"])
        self.assertNotIn("team/Led", result["modules"])
        messages = self.messages(result, "error")
        self.assertTrue(
            any("team/Gone from xrobot.lock is not checked out" in m for m in messages), messages
        )
        self.assertTrue(any("team/Led is checked out at" in m for m in messages), messages)

    def test_a_missing_lock_is_reported(self):
        (self.root / "xrobot.lock").unlink()
        result = describe(self.project)
        self.assertEqual(
            (result["lock"]["status"], result["lock"]["present"], result["modules"]),
            ("absent", False, {}),
        )
        self.assertIn(
            "xrobot.lock does not exist; run `xrobot setup`", self.messages(result, "error")
        )

    def test_entry_problems_are_reported(self):
        (self.root / "User/app_main.cpp").unlink()
        result = describe(self.project)
        self.assertIsNone(result["entry"])
        self.assertEqual(result["registrations"], [])
        self.assertTrue(
            any(
                d["scope"] == "registrations"
                and "No source under User/ calls XROBOT_MAIN" in d["message"]
                for d in result["diagnostics"]
            ),
            result["diagnostics"],
        )
        self.entry("int main() { XR_REGISTER(a, X, Y); XROBOT_MAIN(); }\n")
        self.assertTrue(
            any(
                "registers one type per name" in m
                for m in self.messages(describe(self.project), "error")
            )
        )
