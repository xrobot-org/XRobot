"""BSP 的组成（xrobot.project）：根目录、入口源文件、配置和生成头文件的输入清单。
The layout of a BSP (xrobot.project): root, entry source, configurations and the input list of
the generated header.
"""

import os
from pathlib import Path
from unittest import mock

from fixtures import BspTestCase, TempDirTestCase

from xrobot.project import (
    HeaderInputs,
    Project,
    ProjectError,
    find_root,
    header_banner,
    input_digest,
    path_order,
    read_header_inputs,
)
from xrobot.type_index import module_headers

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

    def test_chinese_entry_errors_use_the_glossary_terms(self):
        # 以前中文报错写“登记硬件”“一个 BSP 只能有一个入口”，与术语表的“注册”“入口源文件”不同。
        # The Chinese errors used to say 登记 and 入口 instead of the glossary terms 注册 and
        # 入口源文件.
        self.write("User/app_main.cpp", "int main() {}\n")
        with mock.patch.dict(os.environ, XR_LANG="zh"):
            with self.assertRaises(ProjectError) as context:
                Project(self.root).entry()
            self.assertEqual(
                str(context.exception),
                "User/ 下没有源文件调用 XROBOT_MAIN()；入口源文件应在用 XR_REGISTER 注册硬件后"
                "调用它一次",
            )
            self.write("User/a.cpp", MAIN)
            self.write("User/b.cpp", MAIN)
            with self.assertRaises(ProjectError) as context:
                Project(self.root).entry()
            self.assertEqual(
                str(context.exception),
                "User/ 下有多个源文件调用 XROBOT_MAIN()：User/a.cpp、User/b.cpp；一个 BSP 只能有"
                "一个入口源文件",
            )

    def test_several_callers_are_listed_in_plain_case_sensitive_order(self):
        # Path 的比较在 Windows 上不区分大小写；按 / 分隔的写法比较，两个系统的顺序相同。
        # Comparing Path objects ignores case on Windows; the / separated spelling gives one
        # order on both systems.
        for name in ("a.cpp", "B.cpp", "c.cpp", "A_sub/d.cpp", "Z.cpp"):
            self.write(f"User/{name}", MAIN)
        with self.assertRaises(ProjectError) as context:
            Project(self.root).entry()
        self.assertEqual(
            str(context.exception),
            "Several sources under User/ call XROBOT_MAIN(): User/A_sub/d.cpp, User/B.cpp, "
            "User/Z.cpp, User/a.cpp, User/c.cpp; a BSP has exactly one entry",
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
        # 输入清单在文件末尾，前面隔一个空行；文件开头只有说明行。
        # The input list ends the file after a blank line; the start has only the banner.
        self.assertEqual(
            lines[-7:],
            [
                "",
                '// xrobot: config "xrobot.yaml"',
                '// xrobot: depends "../xrobot.lock"',
                '// xrobot: entry "app_main.cpp"',
                '// xrobot: depends "../Modules/team/Foo/Foo.hpp"',
                '// xrobot: depends "../Modules/team/Foo/FooTypes.hpp"',
                f"// xrobot: digest {digest}",
            ],
        )
        self.assertEqual(lines[0], header_banner("User/xrobot.yaml"))
        self.assertFalse(any(line.startswith("// xrobot:") for line in lines[:-7]))
        self.assertEqual(
            read_header_inputs(self.root / "User/xrobot_main.hpp"),
            HeaderInputs("xrobot.yaml", inputs, digest),
        )

    def test_the_stamp_lists_inputs_in_plain_case_sensitive_order(self):
        # 同一个 BSP 在 Windows 和 Linux 上生成同样的头文件和摘要：按相对路径的写法逐字符比较，
        # 大写字母在小写字母之前，不论文件系统是否区分大小写。
        # One BSP generates the same header and digest on Windows and Linux: the relative
        # paths are compared character by character, uppercase before lowercase, whether or
        # not the filesystem distinguishes case.
        self.module("CMD", "class CMD { public: CMD() {} };")
        self.module("Chassis", "class Chassis { public: Chassis() {} };")
        self.module("alpha", "class alpha { public: alpha() {} };")
        self.module("Delta", "class Delta { public: Delta() {} };", owner="Zeta")
        self.module("Echo", "class Echo { public: Echo() {} };", owner="acme")
        self.module(
            "Mixed",
            "class Mixed { public: Mixed() {} };",
            extra_headers={"ax.hpp": "", "Ax2.hpp": ""},
        )
        self.generate({"modules": []})
        text = self.read("User/xrobot_main.hpp")
        depends = [line for line in text.splitlines() if line.startswith("// xrobot: depends")]
        self.assertEqual(
            depends,
            [
                '// xrobot: depends "../xrobot.lock"',
                '// xrobot: depends "../Modules/Zeta/Delta/Delta.hpp"',
                '// xrobot: depends "../Modules/acme/Echo/Echo.hpp"',
                '// xrobot: depends "../Modules/team/CMD/CMD.hpp"',
                '// xrobot: depends "../Modules/team/Chassis/Chassis.hpp"',
                '// xrobot: depends "../Modules/team/Foo/Foo.hpp"',
                '// xrobot: depends "../Modules/team/Foo/FooTypes.hpp"',
                '// xrobot: depends "../Modules/team/Mixed/Ax2.hpp"',
                '// xrobot: depends "../Modules/team/Mixed/Mixed.hpp"',
                '// xrobot: depends "../Modules/team/Mixed/ax.hpp"',
                '// xrobot: depends "../Modules/team/alpha/alpha.hpp"',
            ],
        )
        recorded = read_header_inputs(self.root / "User/xrobot_main.hpp")
        paths = [path for kind, path in recorded.inputs if kind == "depends"]
        self.assertEqual(paths[1:], sorted(paths[1:]))
        self.assertEqual(
            recorded.digest, input_digest(self.root / "User/xrobot_main.hpp", recorded.inputs)
        )

    def test_the_module_headers_are_ordered_by_their_plain_spelling(self):
        self.module(
            "Mixed",
            "class Mixed { public: Mixed() {} };",
            extra_headers={"ax.hpp": "", "Ax2.hpp": "", "Zed.hpp": ""},
        )
        modules = self.load_modules()
        names = [path.name for path in module_headers({"team/Mixed": modules["team/Mixed"]})]
        self.assertEqual(names, ["Ax2.hpp", "Mixed.hpp", "Zed.hpp", "ax.hpp"])

    def test_the_sort_key_is_the_slash_spelling_with_case_significant(self):
        from pathlib import PurePosixPath, PureWindowsPath

        for kind in (PurePosixPath, PureWindowsPath):
            paths = [kind("Modules/b/x.hpp"), kind("Modules/B/y.hpp"), kind("Modules/a/z.hpp")]
            with self.subTest(kind=kind.__name__):
                self.assertEqual(
                    [path_order(path) for path in sorted(paths, key=path_order)],
                    ["Modules/B/y.hpp", "Modules/a/z.hpp", "Modules/b/x.hpp"],
                )
        self.assertEqual(path_order(PureWindowsPath(r"a\B\c.hpp")), "a/B/c.hpp")

    def test_a_path_type_that_compares_case_blind_does_not_change_the_stamp(self):
        # 模拟 Windows 上 Path 的比较：生成器不依赖 Path 之间的比较。
        # Simulate how Path compares on Windows: the generator does not depend on comparing
        # Path objects.
        class CaseBlind(type(Path())):
            """按小写比较的路径，像 Windows 上的 Path。
            A path that compares in lower case, like Path on Windows.
            """

            def __lt__(self, other):
                return str(self).casefold() < str(other).casefold()

        self.module("CMD", "class CMD { public: CMD() {} };")
        self.module("alpha", "class alpha { public: alpha() {} };")
        self.module("Chassis", "class Chassis { public: Chassis() {} };")
        modules = self.load_modules()
        blind = [CaseBlind(path) for path in module_headers(modules)]
        with mock.patch("xrobot.generate_main.module_headers", return_value=blind):
            self.generate({"modules": []})
        depends = [
            line
            for line in self.read("User/xrobot_main.hpp").splitlines()
            if line.startswith("// xrobot: depends")
        ]
        self.assertEqual(depends[1:], sorted(depends[1:]))
        self.assertLess(
            depends.index('// xrobot: depends "../Modules/team/CMD/CMD.hpp"'),
            depends.index('// xrobot: depends "../Modules/team/alpha/alpha.hpp"'),
        )

    def load_modules(self):
        """BSP 中锁定的全部模块。
        Every Module locked in the BSP.
        """
        from xrobot.generate_main import load_modules

        return load_modules(self.project)

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

    def test_the_input_list_is_read_wherever_it_is_in_the_file(self):
        # 当前版本写在文件末尾；早期的头文件把它写在开头，说明行在它之前或之后，或没有说明。
        # The current version writes it at the end; earlier headers wrote it at the start, with
        # the notice before or after it, or without a notice.
        markers = '// xrobot: config "xrobot.yaml"\n// xrobot: depends "app_main.cpp"\n'
        old_notice = "// Generated by `xrobot gen` or `xrobot setup`; do not edit by hand."
        code = "static int x;\n"
        for text in (
            "#pragma once\n" + markers,
            "#pragma once\n" + markers + old_notice + "\n" + code,
            "#pragma once\n" + old_notice + "\n" + markers + code,
            header_banner("User/xrobot.yaml") + "\n#pragma once\n" + code + "\n" + markers,
        ):
            with self.subTest(text=text):
                self.write("User/xrobot_main.hpp", text)
                self.assertEqual(
                    read_header_inputs(self.root / "User/xrobot_main.hpp"),
                    HeaderInputs(
                        "xrobot.yaml",
                        [("config", "xrobot.yaml"), ("depends", "app_main.cpp")],
                        None,
                    ),
                )

    def test_the_digest_covers_the_inputs_and_not_the_text_of_the_header(self):
        # 印记行的位置和头文件其余部分的内容都不影响摘要和新旧判断。
        # Neither the position of the stamp lines nor the rest of the header changes the digest
        # or the freshness.
        self.generate({"modules": [{"module": "Foo", "id": "foo"}]})
        header = self.root / "User/xrobot_main.hpp"
        recorded = read_header_inputs(header)
        lines = header.read_text(encoding="utf-8").splitlines()
        stamps = [line for line in lines if line.startswith("// xrobot:")]
        moved = ["// moved", *stamps, "#pragma once", *[x for x in lines if x not in stamps]]
        header.write_text("\n".join(moved) + "\n", encoding="utf-8")
        self.assertEqual(read_header_inputs(header), recorded)
        self.assertEqual(self.project.header_state()["status"], "fresh")

    def test_a_product_in_a_subdirectory_is_named_relative_to_the_header(self):
        self.generate({"modules": []}, name="products/hero.yaml")
        self.assertIn('// xrobot: config "products/hero.yaml"', self.header_lines())
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
        types = self.root / "Modules/team/Foo/FooTypes.hpp"
        before = types.read_bytes()
        types.unlink()
        self.assertEqual(
            project.header_state(),
            {
                "path": "User/xrobot_main.hpp",
                "status": "stale",
                "config": "User/xrobot.yaml",
                "missing": ["Modules/team/Foo/FooTypes.hpp"],
            },
        )
        types.write_bytes(before)
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
