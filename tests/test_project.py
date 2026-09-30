"""BSP 的组成（xrobot.project）：根目录、入口源文件、配置和生成头文件的输入清单。
The layout of a BSP (xrobot.project): root, entry source, configurations and the input list of
the generated header.
"""

import os
from pathlib import Path
from unittest import mock

from fixtures import BspTestCase, TempDirTestCase

from xrobot.project import (
    HEADER_NOTICE,
    Project,
    ProjectError,
    find_root,
    read_header_inputs,
)

MAIN = '#include "xrobot_main.hpp"\nint main() { XROBOT_MAIN(); }\n'


class RootDiscovery(TempDirTestCase):
    """查找 BSP 根目录。
    Finding the BSP root.
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp / "bsp"
        self.write("Modules/modules.yaml", "modules: []\n")

    def test_the_nearest_directory_with_modules_yaml_is_the_root(self):
        nested = self.root / "User" / "products" / "deep"
        nested.mkdir(parents=True)
        # User/Modules 没有 modules.yaml，不是根目录。
        # User/Modules has no modules.yaml, so it is not a root.
        (self.root / "User" / "Modules").mkdir()
        self.assertEqual(find_root(nested), self.root)
        self.assertEqual(find_root(self.root), self.root)
        self.assertEqual(Project.discover(nested).root, self.root)
        inner = self.root / "third_party" / "other"
        self.write(inner / "Modules/modules.yaml", "modules: []\n")
        (inner / "User").mkdir()
        self.assertEqual(find_root(inner / "User"), inner)

    def test_missing_bsp_is_an_error_that_suggests_init(self):
        outside = self.tmp / "elsewhere"
        outside.mkdir()
        if any((p / "Modules/modules.yaml").is_file() for p in self.tmp.parents):
            self.skipTest("a directory above the temporary directory is itself a BSP")
        with self.assertRaises(ProjectError) as context:
            find_root(outside)
        self.assertEqual(
            str(context.exception),
            f"No XRobot BSP found at or above {outside} (no Modules/modules.yaml); run "
            "`xrobot init` in the BSP root to create one",
        )


class Entry(TempDirTestCase):
    """找到调用 XROBOT_MAIN() 的入口源文件。
    Finding the entry source that calls XROBOT_MAIN().
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp / "bsp"
        self.write("Modules/modules.yaml", "modules: []\n")
        (self.root / "User").mkdir()

    def test_the_one_caller_under_user_is_the_entry(self):
        self.write("User/other.cpp", "void helper() {}\n")
        for name in ("app_main.cpp", "app/main.c", "app/main.cc", "app/main.cxx"):
            with self.subTest(name=name):
                path = self.write("User/" + name, "void f() { XROBOT_MAIN(); }\n")
                self.assertEqual(Project(self.root).entry(), path)
                path.unlink()

    def test_no_caller_or_several_callers_are_errors(self):
        # Core/ 在 User/ 之外，其中的调用不算入口。
        # Core/ is outside User/, so its call is no entry.
        self.write("Core/main.cpp", MAIN)
        self.write("User/app_main.cpp", "int main() {}\n")
        with self.assertRaises(ProjectError) as context:
            Project(self.root).entry()
        self.assertEqual(
            str(context.exception),
            "No source under User/ calls XROBOT_MAIN(); the entry source must call it once "
            "after registering its hardware with XR_REGISTER",
        )
        self.write("User/a.cpp", MAIN)
        self.write("User/b.cpp", MAIN)
        with self.assertRaises(ProjectError) as context:
            Project(self.root).entry()
        self.assertEqual(
            str(context.exception),
            "Several sources under User/ call XROBOT_MAIN(): User/a.cpp, User/b.cpp; a BSP has "
            "exactly one entry",
        )

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


class Configs(TempDirTestCase):
    """User/ 下的应用配置。
    The application configurations under User/.
    """

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
    """生成头文件的输入清单、是否过期，以及选中的配置。
    The input list of the generated header, its freshness and the selected configuration.
    """

    def setUp(self):
        super().setUp()
        self.module(
            "Foo",
            "class Foo { public: Foo() {} };",
            extra_headers={"FooTypes.hpp": "#pragma once\n"},
        )
        self.entry(MAIN)

    def header_lines(self):
        """生成头文件的各行。
        The lines of the generated header.
        """
        return self.read("User/xrobot_main.hpp").splitlines()

    def test_header_names_its_config_and_every_input_relative_to_itself(self):
        self.generate({"modules": [{"module": "Foo", "id": "foo"}]})
        lines = self.header_lines()
        self.assertEqual(
            lines[:7],
            [
                "#pragma once",
                HEADER_NOTICE,
                '// xrobot: config "xrobot.yaml"',
                '// xrobot: depends "../xrobot.lock"',
                '// xrobot: depends "app_main.cpp"',
                '// xrobot: depends "../Modules/team/Foo/Foo.hpp"',
                '// xrobot: depends "../Modules/team/Foo/FooTypes.hpp"',
            ],
        )
        self.assertFalse(lines[7].startswith("// xrobot:"))
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

    def test_headers_written_before_the_notice_moved_are_still_read(self):
        # 早期的头文件没有说明，或把说明放在 // xrobot: 行之后。
        # Earlier headers had no notice, or had it after the // xrobot: lines.
        markers = '// xrobot: config "xrobot.yaml"\n// xrobot: depends "app_main.cpp"\n'
        for text in ("#pragma once\n" + markers, "#pragma once\n" + markers + HEADER_NOTICE):
            with self.subTest(text=text):
                self.write("User/xrobot_main.hpp", text + "\n")
                self.assertEqual(
                    read_header_inputs(self.root / "User/xrobot_main.hpp"),
                    ("xrobot.yaml", ["app_main.cpp"]),
                )

    def test_a_product_in_a_subdirectory_is_named_relative_to_the_header(self):
        self.generate({"modules": []}, name="products/hero.yaml")
        self.assertEqual(self.header_lines()[2], '// xrobot: config "products/hero.yaml"')
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
        # 与 LibXR 的 CMake 检查（IS_NEWER_THAN）一致：时间相等也算过期。
        # As LibXR's CMake check (IS_NEWER_THAN) decides: equal times are stale too.
        header_ns = (self.root / "User/xrobot_main.hpp").stat().st_mtime_ns
        os.utime(self.root / "User/xrobot.yaml", ns=(header_ns, header_ns))
        state = project.header_state()
        self.assertEqual((state["status"], state["newer"]), ("stale", ["User/xrobot.yaml"]))

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
        with self.assertRaisesMessage(
            ProjectError,
            "User/xrobot_main.hpp was generated for User/alt.yaml, which does not exist; "
            "select a configuration with `xrobot gen -c <config>`",
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
                f'// xrobot: config "{Path(os.path.abspath(self.root / "User/xrobot.yaml")).as_posix()}"',
                f'// xrobot: depends "{Path(os.path.abspath(self.root / "xrobot.lock")).as_posix()}"',
            ],
        )
