"""BSP layout (root, entry, configs, generated header inputs) and XR_REGISTER reading."""

import os
import unittest
from pathlib import Path
from unittest import mock

from fixtures import BspTestCase, TempDirTestCase

from xrobot.config import ConfigError
from xrobot.generate_main import read_registrations
from xrobot.project import Project, ProjectError, find_root, read_header_inputs

MAIN = '#include "xrobot_main.hpp"\nint main() { XROBOT_MAIN(); }\n'


class RootDiscovery(TempDirTestCase):
    def setUp(self):
        super().setUp()
        self.root = self.tmp / "bsp"
        self.write("Modules/modules.yaml", "modules: []\n")

    def test_root_is_found_from_a_nested_directory(self):
        nested = self.root / "User" / "products" / "deep"
        nested.mkdir(parents=True)
        self.assertEqual(find_root(nested), self.root)
        self.assertEqual(find_root(self.root), self.root)
        self.assertEqual(Project.discover(nested).root, self.root)

    def test_nearest_bsp_wins(self):
        inner = self.root / "third_party" / "other"
        self.write(inner / "Modules/modules.yaml", "modules: []\n")
        (inner / "User").mkdir()
        self.assertEqual(find_root(inner / "User"), inner)

    def test_missing_bsp_is_an_error_that_suggests_init(self):
        outside = self.tmp / "elsewhere"
        outside.mkdir()
        if any((p / "Modules/modules.yaml").is_file() for p in self.tmp.parents):
            self.skipTest("a directory above the temporary directory is itself a BSP")
        with self.assertRaisesRegex(
            ProjectError, r"No XRobot BSP found .*Modules/modules\.yaml.*xrobot init"
        ):
            find_root(outside)

    def test_a_modules_directory_without_modules_yaml_is_not_a_root(self):
        (self.root / "User" / "Modules").mkdir(parents=True)
        self.assertEqual(find_root(self.root / "User"), self.root)


class Entry(TempDirTestCase):
    def setUp(self):
        super().setUp()
        self.root = self.tmp / "bsp"
        self.write("Modules/modules.yaml", "modules: []\n")
        (self.root / "User").mkdir()

    def test_the_unique_caller_is_the_entry(self):
        path = self.write("User/app_main.cpp", MAIN)
        self.write("User/other.cpp", "void helper() {}\n")
        self.assertEqual(Project(self.root).entry(), path)

    def test_entry_can_be_any_source_suffix_in_any_subdirectory(self):
        for name in ("app/main.c", "app/main.cc", "app/main.cxx"):
            with self.subTest(name=name):
                for old in (self.root / "User").rglob("*"):
                    if old.is_file():
                        old.unlink()
                path = self.write("User/" + name, "void f() { XROBOT_MAIN(); }\n")
                self.assertEqual(Project(self.root).entry(), path)

    def test_no_caller_is_an_error(self):
        self.write("User/app_main.cpp", "int main() {}\n")
        with self.assertRaisesRegex(ProjectError, "No source under User/ calls XROBOT_MAIN"):
            Project(self.root).entry()

    def test_several_callers_are_an_error_naming_them(self):
        self.write("User/a.cpp", MAIN)
        self.write("User/b.cpp", MAIN)
        with self.assertRaisesRegex(ProjectError, "Several sources .*User/a.cpp, User/b.cpp"):
            Project(self.root).entry()

    def test_comments_strings_directives_and_headers_are_not_calls(self):
        self.write("User/app_main.cpp", MAIN)
        self.write(
            "User/notes.cpp",
            "// XROBOT_MAIN();\n/* XROBOT_MAIN() */\n"
            'const char* s = "XROBOT_MAIN()";\n#define RUN XROBOT_MAIN()\n'
            "int XROBOT_MAIN_COUNT = 0;\n",
        )
        self.write("User/app.hpp", "inline void Run() { XROBOT_MAIN(); }\n")
        self.assertEqual(Project(self.root).entry(), self.root / "User/app_main.cpp")

    def test_sources_outside_user_are_not_entries(self):
        self.write("Core/main.cpp", MAIN)
        with self.assertRaises(ProjectError):
            Project(self.root).entry()


class Configs(TempDirTestCase):
    def setUp(self):
        super().setUp()
        self.root = self.tmp / "bsp"
        self.write("Modules/modules.yaml", "modules: []\n")

    def test_every_yaml_under_user_except_libxr_config_is_a_config(self):
        for name in (
            "User/xrobot.yaml",
            "User/products/hero.yaml",
            "User/libxr_config.yaml",
            "Modules/other.yaml",
            "User/notes.txt",
        ):
            self.write(name, "modules: []\n")
        project = Project(self.root)
        self.assertEqual(
            [project.relative(p) for p in project.configs()],
            ["User/products/hero.yaml", "User/xrobot.yaml"],
        )

    def test_no_user_directory_means_no_configs(self):
        self.assertEqual(Project(self.root).configs(), [])


class GeneratedHeaderInputs(BspTestCase):
    def setUp(self):
        super().setUp()
        self.module(
            "Foo",
            "class Foo { public: Foo() {} };",
            extra_headers={"FooTypes.hpp": "#pragma once\n"},
        )
        self.entry(MAIN)

    def header_lines(self):
        return self.read("User/xrobot_main.hpp").splitlines()

    def test_header_names_its_config_and_every_input_relative_to_itself(self):
        self.generate({"modules": [{"module": "Foo", "id": "foo"}]})
        lines = self.header_lines()
        self.assertEqual(
            lines[:6],
            [
                "#pragma once",
                '// xrobot: config "xrobot.yaml"',
                '// xrobot: depends "../xrobot.lock"',
                '// xrobot: depends "app_main.cpp"',
                '// xrobot: depends "../Modules/team/Foo/Foo.hpp"',
                '// xrobot: depends "../Modules/team/Foo/FooTypes.hpp"',
            ],
        )
        self.assertFalse(lines[6].startswith("// xrobot:"))
        self.assertEqual(
            read_header_inputs(self.root / "User/xrobot_main.hpp"),
            (
                "xrobot.yaml",
                [
                    "../xrobot.lock",
                    "app_main.cpp",
                    "../Modules/team/Foo/Foo.hpp",
                    "../Modules/team/Foo/FooTypes.hpp",
                ],
            ),
        )

    def test_a_product_in_a_subdirectory_is_named_relative_to_the_header(self):
        self.generate({"modules": []}, name="products/hero.yaml")
        self.assertEqual(self.header_lines()[1], '// xrobot: config "products/hero.yaml"')
        self.assertEqual(self.project.selected_config(), self.root / "User/products/hero.yaml")

    def test_header_state_follows_file_times(self):
        project = self.project
        self.assertEqual(project.header_state()["status"], "missing")
        self.generate({"modules": []})
        state = project.header_state()
        self.assertEqual((state["status"], state["config"]), ("fresh", "User/xrobot.yaml"))
        header_time = (self.root / "User/xrobot_main.hpp").stat().st_mtime
        for relative in (
            "User/xrobot.yaml",
            "User/app_main.cpp",
            "xrobot.lock",
            "Modules/team/Foo/FooTypes.hpp",
        ):
            with self.subTest(newer=relative):
                path = self.root / relative
                os.utime(path, (header_time + 10, header_time + 10))
                state = project.header_state()
                self.assertEqual(state["status"], "stale")
                self.assertEqual(state["newer"], [relative])
                os.utime(path, (header_time - 10, header_time - 10))
        self.assertEqual(project.header_state()["status"], "fresh")

    def test_a_missing_input_makes_the_header_stale(self):
        self.generate({"modules": []})
        (self.root / "Modules/team/Foo/FooTypes.hpp").unlink()
        state = self.project.header_state()
        self.assertEqual(
            (state["status"], state["missing"]), ("stale", ["Modules/team/Foo/FooTypes.hpp"])
        )

    def test_unparseable_input_lines_make_the_header_unreadable(self):
        for text in (
            "#pragma once\n",
            '#pragma once\n// xrobot: config "a.yaml"\n',
            '#pragma once\n// xrobot: config a.yaml\n// xrobot: depends "b"\n',
            '#pragma once\n// xrobot: config "a"\n// xrobot: config "b"\n// xrobot: depends "c"\n',
            '#pragma once\n// xrobot: config "a"\n// xrobot: input "c"\n',
        ):
            with self.subTest(text=text):
                self.write("User/xrobot_main.hpp", text)
                self.assertEqual(read_header_inputs(self.root / "User/xrobot_main.hpp"), (None, []))
                self.assertEqual(self.project.header_state()["status"], "unreadable")

    def test_selected_config_comes_from_the_header_else_the_default(self):
        project = self.project
        self.assertEqual(project.selected_config(), self.root / "User/xrobot.yaml")
        self.generate({"modules": []}, name="alt.yaml")
        self.assertEqual(project.selected_config(), (self.root / "User/alt.yaml").resolve())
        (self.root / "User/alt.yaml").unlink()
        with self.assertRaisesRegex(
            ProjectError, "generated for User/alt.yaml, which does not exist"
        ):
            project.selected_config()
        self.write("User/xrobot_main.hpp", "#pragma once\n")
        self.assertEqual(project.selected_config(), self.root / "User/xrobot.yaml")

    def test_inputs_on_another_drive_are_written_as_absolute_paths(self):
        with mock.patch(
            "xrobot.project.os.path.relpath", side_effect=ValueError("path is on mount C:")
        ):
            lines = self.project.header_lines(
                self.root / "User/xrobot.yaml", [self.root / "xrobot.lock"]
            )
        self.assertEqual(
            lines,
            [
                '// xrobot: config "{}"'.format(
                    Path(os.path.abspath(self.root / "User/xrobot.yaml")).as_posix()
                ),
                '// xrobot: depends "{}"'.format(
                    Path(os.path.abspath(self.root / "xrobot.lock")).as_posix()
                ),
            ],
        )

    def test_generation_without_a_lock_is_refused(self):
        (self.root / "xrobot.lock").unlink()
        with self.assertRaisesRegex(ValueError, r"xrobot\.lock does not exist; run `xrobot setup`"):
            self.generate({"modules": []})


class Registrations(TempDirTestCase):
    def setUp(self):
        super().setUp()
        self.root = self.tmp

    def read_entry(self, text):
        return read_registrations(self.write("app_main.cpp", text))

    def test_name_type_and_line_are_read(self):
        records = self.read_entry(
            '#include "xrobot_main.hpp"\nvoid f() {\n  XR_REGISTER(led, LibXR::GPIO);\n'
            "  XR_REGISTER(values, std::array<int, 2>);\n}\n"
        )
        self.assertEqual(
            [(r["name"], r["type"], r["line"]) for r in records],
            [("led", "LibXR::GPIO", 3), ("values", "std::array<int, 2>", 4)],
        )

    def test_comments_and_string_literals_are_not_registrations(self):
        records = self.read_entry(
            "// XR_REGISTER(fake, Wrong);\n/* XR_REGISTER(a, B); */\n"
            'const char* s = "XR_REGISTER(x, y)";\nXR_REGISTER(real, int);\n'
        )
        self.assertEqual([r["name"] for r in records], ["real"])

    def test_more_than_one_type_is_rejected_with_the_reference_alias_hint(self):
        with self.assertRaisesRegex(
            ConfigError,
            r"app_main\.cpp:1: XR_REGISTER registers one type per name.*"
            r"declare a reference .*LibXR::CAN& can1 = fdcan1;",
        ):
            self.read_entry("XR_REGISTER(dev, Left, Right);\n")

    def test_a_missing_type_is_rejected(self):
        with self.assertRaisesRegex(ConfigError, "one type per name"):
            self.read_entry("XR_REGISTER(dev);\n")

    def test_conditional_registration_is_rejected(self):
        for directive in ("#if defined(OPTION)", "#ifdef OPTION", "#ifndef OPTION"):
            with (
                self.subTest(directive=directive),
                self.assertRaisesRegex(
                    ConfigError, r"app_main\.cpp:3: XR_REGISTER inside #if/#ifdef/#ifndef"
                ),
            ):
                self.read_entry(f"int x;\n{directive}\nXR_REGISTER(x, int);\n#endif\n")

    def test_registration_after_a_closed_conditional_is_accepted(self):
        records = self.read_entry(
            "#if OPTION\nint y;\n#else\nint z;\n#endif\nint x;\nXR_REGISTER(x, int);\n"
        )
        self.assertEqual([r["name"] for r in records], ["x"])

    def test_registration_inside_a_macro_definition_is_rejected(self):
        with self.assertRaisesRegex(
            ConfigError, "app_main.cpp:1: XR_REGISTER inside a preprocessor directive"
        ):
            self.read_entry("#define REGISTER_ALL XR_REGISTER(x, int)\nint x;\n")

    def test_reference_types_are_rejected(self):
        for cpp_type in ("int&", "LibXR::CAN &", "int&&"):
            with (
                self.subTest(cpp_type=cpp_type),
                self.assertRaisesRegex(ConfigError, "not reference types"),
            ):
                self.read_entry(f"XR_REGISTER(x, {cpp_type});\n")

    def test_duplicate_names_are_rejected(self):
        with self.assertRaisesRegex(ConfigError, r"app_main\.cpp:2: duplicate XR_REGISTER name x"):
            self.read_entry("XR_REGISTER(x, int);\nXR_REGISTER(x, long);\n")

    def test_names_must_be_valid_object_names(self):
        for name, reason in (
            ("class", "keyword"),
            ("xr_led", "prefix reserved"),
            ("ASSERT", "macro"),
        ):
            with (
                self.subTest(name=name),
                self.assertRaisesRegex(ConfigError, f"registration name {name} .*{reason}"),
            ):
                self.read_entry(f"XR_REGISTER({name}, int);\n")

    def test_every_registration_error_is_reported(self):
        with self.assertRaises(ConfigError) as context:
            self.read_entry(
                "XR_REGISTER(a, int&);\nXR_REGISTER(b, X, Y);\nXR_REGISTER(class, int);\n"
            )
        self.assertEqual(len(str(context.exception).splitlines()), 3)

    def test_malformed_invocation_is_rejected(self):
        with self.assertRaisesRegex(ConfigError, "malformed XR_REGISTER"):
            self.read_entry("XR_REGISTER(x, int\n")

    def test_types_declared_by_the_caller_are_marked(self):
        records = self.read_entry(
            "struct Global {};\nvoid f() {\n  struct Local {};\n  Local d; Global g; int n;\n"
            "  XR_REGISTER(d, Local);\n  XR_REGISTER(g, Global);\n  XR_REGISTER(n, int);\n"
            "  XR_REGISTER(p, LibXR::GPIO);\n}\n"
        )
        self.assertEqual(
            {r["name"]: r["caller_view"] for r in records},
            {"d": True, "g": True, "n": False, "p": False},
        )


if __name__ == "__main__":
    unittest.main()
