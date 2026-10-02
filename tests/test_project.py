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
    HeaderInputs,
    Project,
    ProjectError,
    find_root,
    input_digest,
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
        # 以前 STM32CubeMX 工程也只得到上面这句，不知道入口源文件由 libxr 生成。
        # An STM32CubeMX project used to get only the sentence above, without learning that
        # libxr generates the entry source.
        self.write("Board.ioc", "")
        with self.assertRaises(ProjectError) as context:
            Project(self.root).entry()
        self.assertEqual(
            str(context.exception),
            "No source under User/ calls XROBOT_MAIN(); the entry source must call it once "
            "after registering its hardware with XR_REGISTER. For this STM32CubeMX project, "
            "`libxr stm32 setup --xrobot` generates such an entry source, User/app_main.cpp",
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
        inputs = [
            ("config", "xrobot.yaml"),
            ("depends", "../xrobot.lock"),
            ("entry", "app_main.cpp"),
            ("depends", "../Modules/team/Foo/Foo.hpp"),
            ("depends", "../Modules/team/Foo/FooTypes.hpp"),
        ]
        digest = input_digest(self.root / "User/xrobot_main.hpp", inputs)
        self.assertEqual(
            lines[:8],
            [
                "#pragma once",
                HEADER_NOTICE,
                '// xrobot: config "xrobot.yaml"',
                '// xrobot: depends "../xrobot.lock"',
                '// xrobot: entry "app_main.cpp"',
                '// xrobot: depends "../Modules/team/Foo/Foo.hpp"',
                '// xrobot: depends "../Modules/team/Foo/FooTypes.hpp"',
                f"// xrobot: digest {digest}",
            ],
        )
        self.assertFalse(lines[8].startswith("// xrobot:"))
        self.assertEqual(
            read_header_inputs(self.root / "User/xrobot_main.hpp"),
            HeaderInputs("xrobot.yaml", inputs, digest),
        )

    def test_the_digest_matches_the_libxr_check(self):
        # LibXR 的 test/automatic/cmake/xrobot_freshness.cmake 对同样的文件期望同一个摘要；
        # 两边算法不同时，构建和 describe 对同一个头文件给出不同的结论。入口源文件只计
        # XR_REGISTER 调用，回车符不计：CRLF 换行、跨行且带中文注释的调用、类型中的括号，
        # 注释中的调用也计入。
        # LibXR's test/automatic/cmake/xrobot_freshness.cmake expects the same digest for the
        # same files; differing algorithms would make the build and describe disagree about
        # one header. The entry counts only its XR_REGISTER calls, without carriage returns:
        # CRLF line ends, a call over two lines with a Chinese comment, parentheses in a type,
        # and a call inside a comment, which counts too.
        self.write("User/产品/英雄.yaml", "modules:\n  - module: Foo\n    id: foo\n")
        self.write("xrobot.lock", "lock: 1\n")
        self.write(
            "User/app_main.cpp",
            '#include "xrobot_main.hpp"\r\n// 入口\r\nint main() {\r\n'
            "  XR_REGISTER(pin, int);\r\n"
            "  XR_REGISTER(table, /* 表 */\r\n              decltype(storage[0]));\r\n"
            "  // XR_REGISTER(old, int);\r\n  XROBOT_MAIN();\r\n}\r\n",
        )
        self.write("Modules/team/Foo/Foo.hpp", "class Foo { public: Foo() {} };\n")
        inputs = [
            ("config", "产品/英雄.yaml"),
            ("depends", "../xrobot.lock"),
            ("entry", "app_main.cpp"),
            ("depends", "../Modules/team/Foo/Foo.hpp"),
        ]
        self.assertEqual(
            input_digest(self.root / "User/xrobot_main.hpp", inputs),
            "307fb19f0ca77bf9074ff03a636161f750578e17e6ba30c99e4634fede9a30e2",
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
                    HeaderInputs(
                        "xrobot.yaml",
                        [("config", "xrobot.yaml"), ("depends", "app_main.cpp")],
                        None,
                    ),
                )

    def test_a_product_in_a_subdirectory_is_named_relative_to_the_header(self):
        self.generate({"modules": []}, name="products/hero.yaml")
        self.assertEqual(self.header_lines()[2], '// xrobot: config "products/hero.yaml"')
        self.assertEqual(self.project.selected_config(), self.root / "User/products/hero.yaml")

    def test_header_state_follows_content(self):
        # 以前按修改时间判断：git checkout、复制工程和机器之间的时钟差都会使没有改动的输入显得
        # 较新，构建因此失败；改动入口源文件中注册以外的代码也是这样。
        # Freshness used to follow file times: a git checkout, a copied project or clock skew
        # made unchanged inputs look newer and failed the build, and so did editing code of
        # the entry source other than the registrations.
        project = self.project
        self.assertEqual(project.header_state()["status"], "missing")
        self.generate({"modules": []})
        self.assertEqual(
            project.header_state(),
            {
                "path": "User/xrobot_main.hpp",
                "status": "fresh",
                "config": "User/xrobot.yaml",
                "missing": [],
            },
        )
        inputs = (
            "User/xrobot.yaml",
            "User/app_main.cpp",
            "xrobot.lock",
            "Modules/team/Foo/Foo.hpp",
        )
        future = (self.root / "User/xrobot_main.hpp").stat().st_mtime + 3600
        for relative in inputs:
            os.utime(self.root / relative, (future, future))
        self.assertEqual(project.header_state()["status"], "fresh")
        entry = self.root / "User/app_main.cpp"
        entry.write_bytes(entry.read_bytes() + b"// a note outside the registrations\n")
        self.assertEqual(project.header_state()["status"], "fresh")
        for relative, change in zip(
            inputs,
            (b"# note\n", b"static int pin;\nXR_REGISTER(pin, int);\n", b"# note\n", b"// note\n"),
            strict=True,
        ):
            with self.subTest(changed=relative):
                path = self.root / relative
                before = path.read_bytes()
                path.write_bytes(before + change)
                self.assertEqual(project.header_state()["status"], "stale")
                path.write_bytes(before)
                self.assertEqual(project.header_state()["status"], "fresh")
        # 旧版本生成的头文件没有摘要，要重新生成。
        # A header from an older version has no digest and needs regenerating.
        header = self.root / "User/xrobot_main.hpp"
        header.write_bytes(
            b"".join(
                line
                for line in header.read_bytes().splitlines(keepends=True)
                if not line.startswith(b"// xrobot: digest ")
            )
        )
        self.assertEqual(project.header_state()["status"], "stale")

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
            '#pragma once\n// xrobot: config "a"\n// xrobot: depends "c"\n// xrobot: digest 12\n',
        ):
            with self.subTest(text=text):
                self.write("User/xrobot_main.hpp", text)
                self.assertIsNone(read_header_inputs(self.root / "User/xrobot_main.hpp"))
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
        config = self.config({"modules": []})
        entry = self.root / "User/app_main.cpp"
        with mock.patch(
            "xrobot.project.os.path.relpath", side_effect=ValueError("path is on mount C:")
        ):
            lines = self.project.header_lines(config, [self.root / "xrobot.lock", entry], entry)
        inputs = [
            ("config", Path(os.path.abspath(config)).as_posix()),
            ("depends", Path(os.path.abspath(self.root / "xrobot.lock")).as_posix()),
            ("entry", Path(os.path.abspath(entry)).as_posix()),
        ]
        self.assertEqual(
            lines,
            [f'// xrobot: {kind} "{path}"' for kind, path in inputs]
            + [f"// xrobot: digest {input_digest(self.root / 'User/xrobot_main.hpp', inputs)}"],
        )
