"""命令行（xrobot.cli）：参数、帮助、输出、退出码，以及命令是否接到实现上。
The command line (xrobot.cli): arguments, help, output, exit codes and whether each command
reaches its implementation.
"""

import argparse
import io
import json
import os
import subprocess
import sys
from unittest import mock

import yaml
from fixtures import BspTestCase, CliMixin, TempDirTestCase, UpstreamTestCase, manifest_block

from xrobot import __version__
from xrobot.cli import main, parse_value, parser
from xrobot.config import ConfigError, load_config
from xrobot.lock import read_modules_yaml

LED = (
    "namespace LibXR { class GPIO; }\nclass Led { public:\n  struct Param { int cycle = 250; };\n"
    "  Led(LibXR::GPIO& gpio, Param param = {}, float gain = 1.0f) {}\n  void OnMonitor() {} };"
)
MAIN = '#include "xrobot_main.hpp"\nint main() { XR_REGISTER(pin, LibXR::GPIO); XROBOT_MAIN(); }\n'


class Init(CliMixin, TempDirTestCase):
    """init、版本、帮助，以及在 BSP 外运行的命令。
    init, the version, help texts, and commands run outside a BSP.
    """

    def setUp(self):
        super().setUp()
        self.root = self.tmp / "new"
        self.root.mkdir()

    def test_init_creates_a_pinned_bsp_skeleton_once(self):
        out, _ = self.ok("-C", self.root, "init")
        self.assertEqual(
            out.strip(),
            "Created Modules/modules.yaml, Modules/sources.yaml, User/xrobot.yaml, "
            ".gitignore entries, .gitattributes entries",
        )
        self.assertEqual(read_modules_yaml(self.root / "Modules/modules.yaml"), ([], __version__))
        self.assertEqual(
            load_config(self.root / "User/xrobot.yaml"),
            {"modules": [], "settings": {"monitor_sleep_ms": "1000"}},
        )
        sources = yaml.safe_load((self.root / "Modules/sources.yaml").read_text(encoding="utf-8"))
        self.assertEqual(
            [s["url"] for s in sources["sources"]],
            ["https://xrobot.work/xrobot-modules/index.yaml"],
        )
        self.assertEqual(
            (self.root / ".gitignore").read_text(encoding="utf-8").splitlines(),
            ["/User/xrobot_main.hpp", "/Modules/CMakeLists.txt", "/Modules/*/"],
        )
        self.assertEqual(
            (self.root / ".gitattributes").read_text(encoding="utf-8").splitlines(),
            ["* text=auto", "*.sh text eol=lf", "*.bat text eol=crlf"],
        )
        out, _ = self.ok("-C", self.root, "init")
        self.assertEqual(out.strip(), "Created nothing (already initialized)")

    def test_init_appends_ignore_entries_with_the_file_line_endings(self):
        self.write(".gitignore", "build/\r\n*.o")
        self.ok("-C", self.root, "init")
        self.assertEqual(
            (self.root / ".gitignore").read_bytes(),
            b"build/\r\n*.o\r\n/User/xrobot_main.hpp\r\n/Modules/CMakeLists.txt\r\n/Modules/*/\r\n",
        )

    def test_init_keeps_existing_files_and_ignore_entries(self):
        self.write(".gitignore", "build/\n/Modules/CMakeLists.txt\n")
        self.write("User/xrobot.yaml", "modules: []\n")
        self.ok("-C", self.root, "init")
        self.assertEqual(self.read("User/xrobot.yaml"), "modules: []\n")
        self.assertEqual(
            self.read(".gitignore").splitlines(),
            ["build/", "/Modules/CMakeLists.txt", "/User/xrobot_main.hpp", "/Modules/*/"],
        )

    def test_init_adds_only_the_missing_attribute_lines(self):
        # 仓库内统一为 LF、签出时转换；已有的行和换行符保持不变。
        # LF inside the repository, converted on checkout; existing lines and line endings stay.
        self.write(".gitattributes", "*.png binary\r\n* text=auto\r\n*.bat text eol=lf")
        out, _ = self.ok("-C", self.root, "init")
        self.assertIn(".gitattributes entries", out)
        self.assertEqual(
            (self.root / ".gitattributes").read_bytes(),
            b"*.png binary\r\n* text=auto\r\n*.bat text eol=lf\r\n"
            b"*.sh text eol=lf\r\n*.bat text eol=crlf\r\n",
        )
        out, _ = self.ok("-C", self.root, "init")
        self.assertEqual(out.strip(), "Created nothing (already initialized)")

    def test_init_leaves_a_complete_gitattributes_file_alone(self):
        text = "* text=auto\n*.sh text eol=lf\n*.bat text eol=crlf\n*.png binary\n"
        self.write(".gitattributes", text)
        self.ok("-C", self.root, "init")
        self.assertEqual(self.read(".gitattributes"), text)

    def test_version(self):
        out, _ = self.ok("--version")
        self.assertEqual(out.strip(), "xrobot " + __version__)

    def test_the_command_finishes_threads_and_exit_callbacks_and_ends_with_the_code_of_main(self):
        # run 结束整个进程，所以在子进程里调用：main 注册一个 atexit 回调，启动一个稍后才写文件的
        # 非守护线程，返回 5。
        # run ends the whole process, so it is called in a child process: main registers an
        # atexit callback, starts a non-daemon thread that writes its file a little later and
        # returns 5.
        script = (
            "import atexit, sys, threading, time\n"
            "from pathlib import Path\n"
            "import xrobot.cli as cli\n"
            "out = Path(sys.argv[1])\n"
            "def late():\n"
            "    time.sleep(0.2)\n"
            "    (out / 'thread').write_text('done')\n"
            "def main():\n"
            "    atexit.register(lambda: (out / 'atexit').write_text('done'))\n"
            "    threading.Thread(target=late).start()\n"
            "    return 5\n"
            "cli.main = main\n"
            "cli.run()\n"
        )
        result = subprocess.run([sys.executable, "-c", script, str(self.root)], check=False)
        self.assertEqual(result.returncode, 5)
        self.assertEqual((self.root / "atexit").read_text(), "done")
        self.assertEqual((self.root / "thread").read_text(), "done")

    def test_help_describes_every_command_and_action(self):
        out, _ = self.ok("source", "--help")
        for action in (
            "list",
            "search",
            "get",
            "find",
            "create-sources",
            "add-source",
            "create-index",
            "add-index",
        ):
            self.assertIn(action, out)
        out, _ = self.ok("instance", "set", "--help")
        self.assertIn("VALUE is one YAML value, read like a value in the config", out)
        self.assertIn("--json", out)
        self.assertIn("VALUE is JSON whose strings are C++ text", out)
        code, _, err = self.run_cli("source")
        self.assertEqual(code, 2)
        self.assertIn("the following arguments are required: <action>", err)

    def test_every_command_help_opens_with_its_summary(self):
        def commands(words, parent):
            for action in parent._actions:
                if isinstance(action, argparse._SubParsersAction):
                    for choice in action._choices_actions:
                        yield words + [choice.dest], choice.help
                        yield from commands(words + [choice.dest], action.choices[choice.dest])

        found = list(commands([], parser()))
        self.assertEqual(len(found), 26)
        for words, summary in found:
            out, _ = self.ok(*words, "--help")
            opening = out.split("\n\n")[1].replace("\n", " ")
            self.assertTrue(
                opening.startswith(summary[0].upper() + summary[1:] + "."), (words, opening)
            )

    def test_source_needs_a_bsp_or_sources(self):
        if any((p / "Modules/modules.yaml").is_file() for p in self.tmp.parents):
            self.skipTest("a directory above the temporary directory is itself a BSP")
        no_bsp = (
            f"No XRobot BSP found at or above {self.root} (no Modules/modules.yaml); run "
            "`xrobot init` in the BSP root to create one"
        )
        self.fails("source", "list", cwd=self.root, message=no_bsp)
        self.fails(
            "source",
            "--sources",
            self.root / "missing.yaml",
            "list",
            cwd=self.root,
            message=f"{self.root / 'missing.yaml'} does not exist; run "
            "`xrobot source create-sources`",
        )

    def test_commands_outside_a_bsp_fail_with_a_hint(self):
        if any((p / "Modules/modules.yaml").is_file() for p in self.tmp.parents):
            self.skipTest("a directory above the temporary directory is itself a BSP")
        self.fails(
            "gen",
            cwd=self.root,
            message=f"No XRobot BSP found at or above {self.root} (no Modules/modules.yaml); run "
            "`xrobot init` in the BSP root to create one",
        )
        self.assertFalse((self.root / "Modules").exists())


class Commands(CliMixin, BspTestCase):
    """各命令的参数、输出和退出码。
    The arguments, output and exit codes of the commands.
    """

    def setUp(self):
        super().setUp()
        self.module("Led", LED)
        self.entry(MAIN)
        self.config(
            {
                "modules": [
                    {
                        "module": "Led",
                        "id": "led",
                        "args": [{"gpio": "pin"}, {"param": {"cycle": "100"}}, {"gain": "1.0f"}],
                    }
                ]
            }
        )
        (self.root / "User/products").mkdir()

    def test_the_root_is_found_from_a_subdirectory_or_with_C(self):
        out, _ = self.ok("gen", cwd=self.root / "User/products")
        self.assertEqual(out.strip(), "Generated User/xrobot_main.hpp for User/xrobot.yaml")
        (self.root / "User/xrobot_main.hpp").unlink()
        self.ok("-C", self.root / "User/products", "gen", cwd=self.tmp)
        self.assertTrue((self.root / "User/xrobot_main.hpp").is_file())

    def test_gen_selects_a_product_relative_to_the_cwd_or_the_root(self):
        self.config({"modules": []}, name="products/alt.yaml")
        out, _ = self.ok("gen", "-c", "alt.yaml", cwd=self.root / "User/products")
        self.assertEqual(out.strip(), "Generated User/xrobot_main.hpp for User/products/alt.yaml")
        self.assertIn('// xrobot: config "products/alt.yaml"', self.read("User/xrobot_main.hpp"))
        self.ok("gen", "-c", "User/xrobot.yaml", cwd=self.root / "User/products")
        self.assertIn('// xrobot: config "xrobot.yaml"', self.read("User/xrobot_main.hpp"))
        out, _ = self.ok("gen", cwd=self.root)
        self.assertIn("User/xrobot.yaml", out)

    def test_gen_writes_line_directives_unless_told_not_to(self):
        header = self.root / "User/xrobot_main.hpp"
        self.ok("gen")
        self.assertIn('#line 2 "User/xrobot.yaml"', header.read_text(encoding="utf-8"))
        self.ok("gen", "--no-line-directives")
        self.assertNotIn("#line", header.read_text(encoding="utf-8"))

    def test_errors_exit_with_status_1_and_a_message(self):
        self.fails(
            "gen",
            "-c",
            "User/missing.yaml",
            message="User/missing.yaml does not exist; create it with "
            "`xrobot instance -c User/missing.yaml add <owner/Repo>`",
        )
        self.config({"modules": [{"module": "Led", "id": "led", "args": [{"gpio": "other"}]}]})
        self.fails(
            "gen",
            message="User/xrobot.yaml: led: named arguments (gpio) do not match any constructor of "
            "Led; expected one of: (gpio, param, gain)",
        )
        self.config("modules: [\n")
        self.fails(
            "gen",
            message="User/xrobot.yaml:2: YAML syntax error: expected the node content, but found "
            "'<stream end>'",
        )
        self.assertFalse((self.root / "User/xrobot_main.hpp").exists())

    def test_output_is_utf8_whatever_the_platform_encoding(self):
        header = self.write(
            self.tmp / "Zh/Zh.hpp",
            "#pragma once\n" + manifest_block("闪烁") + "class Zh { public: Zh() {} };\n",
        )
        out = io.TextIOWrapper(io.BytesIO(), encoding="gbk")
        err = io.TextIOWrapper(io.BytesIO(), encoding="gbk")
        with mock.patch.object(sys, "stdout", out), mock.patch.object(sys, "stderr", err):
            self.assertEqual(main(["module", "show", str(header)]), 0)
            self.assertEqual(main(["-C", str(self.root), "gen", "-c", "User/缺失.yaml"]), 1)
            out.flush()
            err.flush()
        self.assertIn("闪烁".encode(), out.buffer.getvalue())
        self.assertIn("User/缺失.yaml does not exist".encode(), err.buffer.getvalue())

    def test_output_follows_the_language(self):
        with mock.patch.dict(os.environ, XR_LANG="zh"):
            out, _ = self.ok("--help")
            self.assertTrue(out.startswith("用法：xrobot [-h] [--version] [-C DIR] <command> ..."))
            self.assertIn("解析模块、生成主函数、编辑配置。", out)
            self.fails(
                "gen",
                "-c",
                "User/缺失.yaml",
                message="User/缺失.yaml 不存在；请用 "
                "`xrobot instance -c User/缺失.yaml add <owner/Repo>` 创建",
            )
            self.fails(
                "instance",
                "set",
                "led",
                "id",
                "x",
                message="实例 id 用 `xrobot instance rename` 修改，它会同时更新对该实例的引用",
            )
        out, _ = self.ok("--help")
        self.assertTrue(out.startswith("usage: xrobot"))

    def test_describe_prints_json(self):
        out, _ = self.ok("describe", cwd=self.root / "User")
        result = json.loads(out)
        self.assertEqual((result["schema"], result["config"]), (1, "User/xrobot.yaml"))
        self.assertEqual(result["instances"][0]["id"], "led")

    def test_instance_add_lists_candidates_beside_broken_instances(self):
        # 以前配置里已有识别不了的实例时，instance add 已经写入文件，却仍以失败退出。
        # An instance that could not be resolved used to make instance add exit with an
        # error after it had written the file.
        self.config(
            {
                "modules": [
                    {"module": "Missing", "id": "ghost"},
                    {"id": "no_module"},
                    {"module": "Led", "id": "led", "args": [{"gpio": "pin"}]},
                ]
            }
        )
        out, _ = self.ok("instance", "add", "Led", "--id", "second")
        self.assertEqual(
            out,
            "Added second to User/xrobot.yaml; fill the null values (dependencies) before "
            "generating\n  gpio (LibXR::GPIO&): pin\n",
        )

    def test_instance_add_of_a_template_without_default_arguments(self):
        # 以前模板实参没有默认值时，instance add 写入实例后以失败退出（模板实参必须给出）。
        # Without default template arguments, instance add used to exit with an error
        # (the template argument must be specified) after writing the instance.
        self.module(
            "Raw", "template <typename T>\nclass Raw { public: explicit Raw(T init = T{}) {} };"
        )
        out, _ = self.ok("instance", "add", "Raw", "--id", "raw")
        self.assertEqual(
            out,
            "Added raw to User/xrobot.yaml; fill the null values (dependencies) before "
            "generating\n",
        )
        self.assertEqual(
            load_config(self.root / "User/xrobot.yaml")["modules"][-1],
            {"module": "team/Raw", "id": "raw", "template_args": [None]},
        )

    def test_template_instances_get_their_arguments(self):
        # 以前模板实参填写后 sync 补不出参数，gen 报“参数名 () 与任何构造函数都不匹配”；
        # instance add 也不能直接给出模板实参。
        # After the template arguments were filled in, sync used to add no arguments and gen
        # reported "named arguments () do not match any constructor"; instance add could not
        # take the template arguments either.
        self.module(
            "Raw", "template <typename T>\nclass Raw { public: explicit Raw(T init = T{}) {} };"
        )
        self.ok("instance", "add", "Raw", "--id", "given", "--template-arg", "int")
        self.ok("instance", "add", "Raw", "--id", "later")
        self.ok("instance", "set", "--json", "later", "template_args[0]", '"float"')
        self.ok("sync")
        self.assertEqual(
            load_config(self.root / "User/xrobot.yaml")["modules"][-2:],
            [
                {
                    "module": "team/Raw",
                    "id": "given",
                    "template_args": ["int"],
                    "args": [{"init": "int{}"}],
                },
                {
                    "module": "team/Raw",
                    "id": "later",
                    "template_args": ["float"],
                    "args": [{"init": "float{}"}],
                },
            ],
        )

    def test_instance_add_says_why_candidates_are_missing(self):
        # 以前读不出入口源文件时，每个依赖参数都显示“没有候选”，看不出原因。
        # Without a readable entry source every dependency used to show "no candidate",
        # which did not say why.
        self.entry("int main() {}\n")
        out, _ = self.ok("instance", "add", "Led", "--id", "second")
        self.assertEqual(
            out,
            "Added second to User/xrobot.yaml; fill the null values (dependencies) before "
            "generating\n  Candidates not listed: No source under User/ calls XROBOT_MAIN(); the "
            "entry source must call it once after registering its hardware with XR_REGISTER\n",
        )

    def test_instance_add_names_the_setting_that_generates_ramfs_and_database(self):
        # 以前新生成的 STM32 工程对 RamFS、Database 只显示“没有候选”，要开的设置得另查文档。
        # A newly generated STM32 project used to show only "no candidate" for RamFS and
        # Database; the setting to turn on had to be looked up in the documentation.
        self.module(
            "Store",
            "namespace LibXR { class RamFS; class Database; }\nclass Store { public:\n"
            "  Store(LibXR::RamFS& ramfs, LibXR::Database* database) {}\n"
            "  void OnMonitor() {} };",
        )
        unfilled = (
            "Added {} to User/xrobot.yaml; fill the null values (dependencies) before "
            "generating\n  ramfs (LibXR::RamFS&): no candidate\n"
        )
        out, _ = self.ok("instance", "add", "Store", "--id", "plain")
        self.assertEqual(out, unfilled.format("plain") + "  database (LibXR::Database*): nullptr\n")
        self.write("User/libxr_config.yaml", "terminal_source: ''\n")
        out, _ = self.ok("instance", "add", "Store", "--id", "generated")
        self.assertEqual(
            out,
            unfilled.format("generated")
            + "    set terminal_source in User/libxr_config.yaml, then run "
            "`libxr stm32 setup -d .` to generate and register ramfs\n"
            "  database (LibXR::Database*): nullptr\n"
            "    set database.enable: true in User/libxr_config.yaml, then run "
            "`libxr stm32 setup -d .` to generate and register database\n",
        )
        self.entry(
            '#include "xrobot_main.hpp"\nint main() { XR_REGISTER(ramfs, LibXR::RamFS); '
            "XR_REGISTER(database, LibXR::Database); XROBOT_MAIN(); }\n"
        )
        out, _ = self.ok("instance", "add", "Store", "--id", "registered")
        self.assertEqual(
            out,
            "Added registered to User/xrobot.yaml; fill the null values (dependencies) before "
            "generating\n  ramfs (LibXR::RamFS&): ramfs\n"
            "  database (LibXR::Database*): &database, nullptr\n",
        )

    def test_instance_editing(self):
        # 以前只提示填写空值，可以填写的名字要另外去找。
        # Only the null values used to be pointed out; the names to fill in had to be looked
        # up elsewhere.
        out, _ = self.ok("instance", "add", "Led", "--id", "second")
        self.assertEqual(
            out,
            "Added second to User/xrobot.yaml; fill the null values (dependencies) before "
            "generating\n  gpio (LibXR::GPIO&): pin\n",
        )
        out, _ = self.ok("instance", "set", "second", "args.gpio", "pin")
        # 以前成功时什么也不输出。
        # Nothing used to be printed on success.
        self.assertEqual(out, "Set args.gpio of second in User/xrobot.yaml\n")
        self.ok("instance", "set", "second", "args.gain", "2.5")
        second = load_config(self.root / "User/xrobot.yaml")["modules"][1]
        self.assertEqual(
            second["args"], [{"gpio": "pin"}, {"param": {"cycle": "250"}}, {"gain": "2.5"}]
        )
        self.fails(
            "instance",
            "set",
            "second",
            "args.gain",
            "3",
            "--if-match",
            "0" * 64,
            message="User/xrobot.yaml changed since it was read; reload and retry",
        )
        for argv, ids in (
            (("rename", "second", "backup"), ["led", "backup"]),
            (("remove", "backup"), ["led"]),
        ):
            self.ok("instance", *argv)
            self.assertEqual(
                [i["id"] for i in load_config(self.root / "User/xrobot.yaml")["modules"]], ids
            )

    def test_values_are_read_like_config_values_and_json_is_opt_in(self):
        cases = [
            ("LED_B", "LED_B"),
            ('"LED_B"', '"LED_B"'),
            ("2.0f", "2.0f"),
            ("'{250}'", "{250}"),
            ("'&led'", "&led"),
            ('{cycle: 1, name: "a"}', {"cycle": "1", "name": '"a"'}),
            ("", None),
            ("null", None),
        ]
        for text, value in cases:
            with self.subTest(text=text):
                self.assertEqual(parse_value(text), value)
        self.assertEqual(parse_value('"LED_B"', as_json=True), "LED_B")
        self.assertEqual(parse_value('{"a": "\\"x\\""}', as_json=True), {"a": '"x"'})
        with self.assertRaisesMessage(
            ConfigError, "VALUE is not JSON: Expecting value: line 1 column 1 (char 0)"
        ):
            parse_value("LED_B", as_json=True)
        quotes = (
            "; C++ code that starts with {, [, & or * goes in single quotes inside the shell "
            "quoting, e.g. \"'&grey_0'\""
        )
        with self.assertRaisesMessage(
            ConfigError,
            "VALUE:1: YAML syntax error: expected the node content, but found '<stream end>'"
            + quotes,
        ):
            parse_value("[")
        with self.assertRaisesMessage(
            ConfigError, "VALUE:1: YAML syntax error: mapping values are not allowed here"
        ):
            parse_value("a: b: c")
        self.ok("instance", "set", "led", "args.gain", "2.0f")
        self.ok("instance", "set", "led", "args.param", "'{250}'")
        self.ok("instance", "set", "led", "args.gpio", '"pin2"', "--json")
        self.assertEqual(
            load_config(self.root / "User/xrobot.yaml")["modules"][0]["args"],
            [{"gpio": "pin2"}, {"param": "{250}"}, {"gain": "2.0f"}],
        )
        self.fails(
            "instance",
            "set",
            "led",
            "args.gain",
            "[",
            message="VALUE:1: YAML syntax error: expected the node content, but found "
            "'<stream end>'" + quotes,
        )

    def test_code_that_yaml_reads_as_a_collection_or_an_alias_gets_the_quoting_hint(self):
        # 以前只报 YAML 的错误，看不出要把 C++ 代码放进单引号。
        # Only the YAML error used to be reported, without saying that the C++ code goes in
        # single quotes.
        quotes = (
            "; C++ code that starts with {, [, & or * goes in single quotes inside the shell "
            "quoting, e.g. \"'&grey_0'\""
        )
        for value, error in (
            ('{{"topic1", "rx"}, {"topic2", "rx"}}', "VALUE:1: mapping keys must be plain names"),
            (
                "&led",
                "VALUE:1: YAML anchors and aliases are not allowed; reference instances by id "
                "and share values through constexprs",
            ),
            (
                "*led",
                "VALUE:1: YAML anchors and aliases are not allowed; reference instances by id "
                "and share values through constexprs",
            ),
        ):
            with self.subTest(value=value):
                self.fails("instance", "set", "led", "args.gain", value, message=error + quotes)
        self.ok("instance", "set", "led", "args.gain", "'&led'")
        with mock.patch.dict(os.environ, XR_LANG="zh"):
            self.fails(
                "instance",
                "set",
                "led",
                "args.param",
                '{{"topic1", "rx"}}',
                message="VALUE:1: 映射的键必须是普通名字；以 {、[、&、* 开头的 C++ 代码要放在单引号中，"
                "单引号写在 shell 的引号之内，例如 \"'&grey_0'\"",
            )

    def test_instance_add_writes_to_the_selected_product(self):
        self.config({"modules": []}, name="products/alt.yaml")
        self.ok("gen", "-c", "User/products/alt.yaml")
        self.ok("instance", "add", "Led")
        self.assertEqual(
            [i["id"] for i in load_config(self.root / "User/products/alt.yaml")["modules"]],
            ["led_0"],
        )
        self.ok("instance", "-c", "User/xrobot.yaml", "add", "Led")
        self.assertEqual(
            [i["id"] for i in load_config(self.root / "User/xrobot.yaml")["modules"]],
            ["led", "led_0"],
        )

    def test_module_requests(self):
        out, _ = self.ok("module", "add", "team/Other")
        self.assertEqual(out, "Added team/Other; run `xrobot setup` to fetch it\n")
        self.assertEqual(
            read_modules_yaml(self.root / "Modules/modules.yaml"),
            ([{"id": "team/Other", "ref": "same-or-dev"}], __version__),
        )
        self.fails(
            "module",
            "add",
            "team/Other",
            message=f"team/Other is already requested in {self.root / 'Modules/modules.yaml'}",
        )
        out, _ = self.ok("module", "remove", "team/Other")
        self.assertEqual(out, "Removed team/Other; run `xrobot setup` to update xrobot.lock\n")
        self.assertEqual(read_modules_yaml(self.root / "Modules/modules.yaml")[0], [])

    def test_module_show_prints_the_manifest_and_constructors(self):
        for target in (self.root / "Modules/team/Led", "."):
            with self.subTest(target=str(target)):
                out, _ = self.ok("module", "show", target, cwd=self.root / "Modules/team/Led")
                self.assertIn("{}", out)
                self.assertRegex(
                    out,
                    r"Led\.hpp:\d+: Led\(LibXR::GPIO& gpio, Param param = \{\}, float gain = 1\.0f\)",
                )

    def test_a_misspelled_module_gets_the_closest_candidates(self):
        self.fails(
            "module",
            "show",
            "team/Lde",
            message="team/Lde: not a file or folder; Module not found: team/Lde; did you mean team/Led?",
        )

    def test_module_show_takes_the_id_of_a_locked_module(self):
        for target in ("team/Led", "Led"):
            with self.subTest(target=target):
                out, _ = self.ok("module", "show", target, cwd=self.root / "User")
                self.assertRegex(out, r"Led\.hpp:\d+: Led\(LibXR::GPIO& gpio")
        self.fails(
            "module",
            "show",
            "team/Missing",
            message="team/Missing: not a file or folder; Module not found: team/Missing",
        )

    def test_format_check_and_rewrite(self):
        self.config(
            "modules:\n- module: Led\n  id: led\n  args:\n  - gpio: pin\n  - param: {cycle: 1}\n  - gain: 1.0f\n"
        )
        self.fails(
            "format",
            "--check",
            message="Found 1 file not in the canonical layout; run `xrobot format`",
        )
        out, _ = self.run_cli("format", "--check")[1:]
        self.assertIn("needs formatting: User/xrobot.yaml", out)
        out, _ = self.ok("format")
        self.assertEqual(out.strip(), "formatted: User/xrobot.yaml")
        self.ok("format", "--check")
        self.assertIn("\n  - module: Led\n", self.read("User/xrobot.yaml"))

    def test_sync_prints_the_diff(self):
        self.module("Led", LED.replace("int cycle = 250;", "int cycle = 250; int phase = 0;"))
        out, _ = self.ok("sync")
        self.assertIn("+++ User/xrobot.yaml", out)
        self.assertIn("+          phase:", out.replace("'0'", "").replace(" 0", ""))
        self.assertEqual(self.ok("sync")[0], "")

    def test_editing_commands_refuse_configurations_before_1_0(self):
        # 以前 format 照样重排旧文件、sync 和 setup --update 输出不写入的改动、instance add 以
        # traceback 结束。
        # format used to re-indent old files, sync and setup --update printed changes they
        # never wrote and instance add ended in a traceback.
        old = "global_settings:\n  monitor_sleep_ms: 1000\nmodules:\n- id: led\n  name: Led\n"
        self.config(old, name="products/old.yaml")
        before = {p: p.read_bytes() for p in (self.root / "User").rglob("*.yaml")}
        message = (
            "User/products/old.yaml: this configuration uses the format of XRobot before 1.0 "
            "(global_settings, name/constructor_args); XRobot 1.0 lists each instance as "
            "module, id and args; replace the content of the file with `modules: []` (or "
            "delete the file and run `xrobot init`), then recreate the instances with "
            "`xrobot instance -c User/products/old.yaml add`"
        )
        for argv in (
            ("format",),
            ("format", "--check"),
            ("sync",),
            ("setup", "--update"),
            ("instance", "-c", "User/products/old.yaml", "add", "Led", "--id", "second"),
            ("instance", "-c", "User/products/old.yaml", "set", "led", "args.gpio", "pin"),
            ("instance", "-c", "User/products/old.yaml", "rename", "led", "other"),
            ("instance", "-c", "User/products/old.yaml", "remove", "led"),
        ):
            with self.subTest(argv=argv):
                self.fails(*argv, message=message)
                self.assertEqual({p: p.read_bytes() for p in before}, before)
        with mock.patch.dict(os.environ, XR_LANG="zh"):
            self.fails(
                "format",
                message="User/products/old.yaml: 这份配置使用的是 XRobot 1.0 以前的格式"
                "（global_settings、name/constructor_args）；XRobot 1.0 中每个实例写成 module、id "
                "和 args；请把文件内容换成 `modules: []`（或删除文件后运行 `xrobot init`），再用 "
                "`xrobot instance -c User/products/old.yaml add` 重新添加实例",
            )

    def test_format_reports_a_yaml_syntax_error_without_a_traceback(self):
        # 以前 format 遇到 YAML 语法错误时以 ruamel 的 traceback 结束。
        # format used to end in a ruamel traceback on a YAML syntax error.
        self.config("modules: [\n")
        self.fails(
            "format",
            message="User/xrobot.yaml:2: YAML syntax error: expected the node content, but found "
            "'<stream end>'",
        )

    def test_gen_says_when_the_header_is_unchanged(self):
        # 以前内容没变、文件没有重写时也输出 Generated。
        # Generated used to be printed also when the content was the same and the file was
        # not rewritten.
        header = self.root / "User/xrobot_main.hpp"
        out, _ = self.ok("gen")
        self.assertEqual(out, "Generated User/xrobot_main.hpp for User/xrobot.yaml\n")
        written = header.stat().st_mtime_ns
        out, _ = self.ok("gen")
        self.assertEqual(out, "User/xrobot_main.hpp for User/xrobot.yaml is unchanged\n")
        self.assertEqual(header.stat().st_mtime_ns, written)
        out, _ = self.ok("gen", "--no-line-directives")
        self.assertEqual(out, "Generated User/xrobot_main.hpp for User/xrobot.yaml\n")
        with mock.patch.dict(os.environ, XR_LANG="zh"):
            out, _ = self.ok("gen", "--no-line-directives")
        self.assertEqual(out, "为 User/xrobot.yaml 生成的 User/xrobot_main.hpp 未变化\n")

    def test_instance_add_asks_to_fill_values_only_when_some_are_null(self):
        # 以前没有待填的值时也提示“填写值为空的依赖参数”。
        # The prompt to fill the null values used to appear also when nothing was null.
        self.module("Probe", "class Probe { public: explicit Probe(int n = 1) {} };")
        out, _ = self.ok("instance", "add", "Probe", "--id", "probe")
        self.assertEqual(out, "Added probe to User/xrobot.yaml\n")
        self.module("Counter", "class Counter { public: explicit Counter(int n) {} };")
        out, _ = self.ok("instance", "add", "Counter", "--id", "counter")
        self.assertEqual(
            out,
            "Added counter to User/xrobot.yaml; fill the null values (dependencies) before "
            "generating\n",
        )

    def test_registration_errors_name_the_entry_source_from_the_root(self):
        # 以前只写文件名 app_main.cpp，而配置的报错写的是 User/xrobot.yaml。
        # Only the file name app_main.cpp used to be given, while errors in the configuration
        # say User/xrobot.yaml.
        self.entry(
            '#include "xrobot_main.hpp"\nint main() {\n  XR_REGISTER(pin, LibXR::GPIO);\n'
            "  XR_REGISTER(pin, LibXR::GPIO);\n  XROBOT_MAIN();\n}\n"
        )
        self.fails("gen", message="User/app_main.cpp:4: duplicate XR_REGISTER name pin")
        out, _ = self.ok("instance", "add", "Led", "--id", "second")
        self.assertIn(
            "  Candidates not listed: User/app_main.cpp:4: duplicate XR_REGISTER name pin\n", out
        )
        result = json.loads(self.ok("describe")[0])
        self.assertIn(
            "User/app_main.cpp:4: duplicate XR_REGISTER name pin",
            json.dumps(result["diagnostics"]),
        )


class Setup(CliMixin, UpstreamTestCase):
    """setup、check-module 和 source 命令。
    The setup, check-module and source commands.
    """

    def setUp(self):
        super().setUp()
        self.led = self.upstream("team/Led")
        self.commit(self.led, [], "led")
        self.configure(["team/Led@master"])
        self.write(
            self.root / "User/app_main.cpp",
            '#include "xrobot_main.hpp"\nint main() { XROBOT_MAIN(); }\n',
        )
        self.write(self.root / "User/xrobot.yaml", "modules:\n  - module: team/Led\n    id: led\n")

    def test_setup_resolves_checks_every_config_and_generates_the_selected_one(self):
        self.write(self.root / "User/products/alt.yaml", "modules: []\n")
        out, err = self.ok("setup", cwd=self.root / "User")
        self.assertIn("Resolved 1 Module commit\n", out)
        self.assertIn("Checked 2 configs; generated User/xrobot_main.hpp for User/xrobot.yaml", out)
        self.assertEqual(err, "")
        self.assertTrue((self.root / "xrobot.lock").is_file())
        self.assertIn(
            "static Led led;", (self.root / "User/xrobot_main.hpp").read_text(encoding="utf-8")
        )
        self.ok("gen", "-c", "User/products/alt.yaml")
        self.ok("setup", "--frozen")
        self.assertIn(
            '// xrobot: config "products/alt.yaml"',
            (self.root / "User/xrobot_main.hpp").read_text(encoding="utf-8"),
        )

    def test_setup_writes_line_directives_unless_told_not_to(self):
        header = self.root / "User/xrobot_main.hpp"
        self.ok("setup")
        self.assertIn('#line 2 "User/xrobot.yaml"', header.read_text(encoding="utf-8"))
        self.ok("setup", "--frozen", "--no-line-directives")
        self.assertNotIn("#line", header.read_text(encoding="utf-8"))

    def test_setup_fails_when_any_config_is_invalid(self):
        self.write(
            self.root / "User/products/a.yaml", "modules:\n  - {module: team/Led, id: Led}\n"
        )
        self.write(
            self.root / "User/products/b.yaml", "modules:\n  - {module: team/Missing, id: m}\n"
        )
        err = self.fails("setup")
        self.assertIn("User/products/a.yaml: Led: instance id Led is also a class name", err)
        self.assertIn("User/products/b.yaml: m: Module not found: team/Missing", err)
        self.assertFalse((self.root / "User/xrobot_main.hpp").exists())

    def test_frozen_or_offline_without_a_lock_fails(self):
        for option in ("--frozen", "--offline"):
            self.fails(
                "setup",
                option,
                message="xrobot.lock does not exist; run `xrobot setup` once without --frozen or "
                "--offline",
            )

    def test_a_git_timeout_is_reported_without_a_traceback(self):
        timeout = subprocess.TimeoutExpired(["git", "fetch"], 300)
        with mock.patch("xrobot.git.subprocess.run", side_effect=timeout):
            self.fails(
                "setup",
                message=f"Git did not finish within 300 s in {self.root}: rev-parse "
                "--is-inside-work-tree",
            )

    def test_setup_says_when_the_header_is_unchanged(self):
        # 以前内容没变时也输出 generated。
        # generated used to be printed also when the content was the same.
        self.ok("setup")
        out, _ = self.ok("setup")
        self.assertEqual(
            out,
            "Resolved 1 Module commit\nChecked 1 config; User/xrobot_main.hpp for "
            "User/xrobot.yaml is unchanged\n",
        )

    def test_missing_git_is_reported_in_one_line(self):
        # 以前只输出 [Errno 2] No such file or directory: 'git'。
        # Only [Errno 2] No such file or directory: 'git' used to be printed.
        missing = FileNotFoundError(2, "No such file or directory", "git")
        with mock.patch("xrobot.git.subprocess.run", side_effect=missing):
            self.fails(
                "setup",
                message="git was not found on PATH; XRobot fetches the Modules and reads their "
                "commits with Git",
            )

    def test_instance_add_of_a_module_outside_the_lock_names_the_commands(self):
        # 以前报“找不到模块”，并建议另一个模块。
        # "Module not found" used to be reported, with another Module suggested.
        self.upstream("team/Imu")
        self.ok("setup")
        self.fails(
            "instance",
            "add",
            "team/Imu",
            message="team/Imu is not in xrobot.lock; run `xrobot module add team/Imu` and "
            "`xrobot setup` first",
        )
        self.ok("module", "add", "team/Imu@master")
        self.fails(
            "instance",
            "add",
            "Imu",
            message="team/Imu is requested in Modules/modules.yaml but not in xrobot.lock yet; "
            "run `xrobot setup` first",
        )
        self.fails(
            "instance", "add", "other/Gyroscope", message="Module not found: other/Gyroscope"
        )
        self.assertEqual(
            self.read(self.root / "User/xrobot.yaml"),
            "modules:\n  - module: team/Led\n    id: led\n",
        )
        with mock.patch.dict(os.environ, XR_LANG="zh"):
            self.fails(
                "instance",
                "add",
                "team/Imu",
                message="Modules/modules.yaml 已请求 team/Imu，但它还不在 xrobot.lock 中；"
                "请先运行 `xrobot setup`",
            )

    def test_a_different_tool_pin_is_a_warning_and_an_error_when_frozen(self):
        self.configure(["team/Led@master"], pin="0.9.0")
        _, err = self.ok("setup")
        warning = f"warning: installed XRobot {__version__} differs from the pinned 0.9.0"
        self.assertIn(warning, err)
        _, err = self.ok("gen")
        self.assertIn(warning, err)
        self.fails(
            "setup",
            "--frozen",
            message="installed XRobot 1.0.0 differs from the pinned 0.9.0 (--frozen requires the "
            "pinned version)",
        )
        self.configure(["team/Led@master"], pin="0123456789abcdef0123456789abcdef01234567")
        _, err = self.ok("setup", "--frozen")
        self.assertEqual(err, "")
        self.configure(["team/Led@master"], pin=None)
        _, err = self.ok("setup")
        self.assertIn(
            f"warning: Modules/modules.yaml does not pin XRobot; add `xrobot: {__version__}`", err
        )

    def test_setup_update_syncs_config_fields(self):
        self.ok("setup")
        self.write(
            self.led / "Led.hpp",
            "#pragma once\n"
            + manifest_block()
            + "class Led { public: explicit Led(int period = 5) {} };\n",
        )
        from fixtures import run_git

        run_git(self.led, "commit", "-q", "-am", "new parameter")
        out, _ = self.ok("setup", "--update")
        self.assertIn("+      - period: 5", out.replace("'5'", "5"))
        self.assertEqual(
            load_config(self.root / "User/xrobot.yaml")["modules"][0]["args"], [{"period": "5"}]
        )

    def test_check_module_writes_a_probe_offline(self):
        self.ok("setup")
        out, _ = self.ok("check-module", "team/Led", "-o", self.tmp / "probe.cpp", "--offline")
        self.assertIn("Generated", out)
        self.assertIn("static Led module_0;", (self.tmp / "probe.cpp").read_text(encoding="utf-8"))

    def test_source_queries(self):
        board = self.upstream("team/Board", kind="bsp")
        out, _ = self.ok("source", "list")
        self.assertEqual(out, f"team/Board [bsp] {board}\nteam/Led [module] {self.led}\n")
        out, _ = self.ok("source", "list", "--type", "module")
        self.assertEqual(out, f"team/Led [module] {self.led}\n")
        out, _ = self.ok("source", "get", "Led")
        self.assertEqual(yaml.safe_load(out)["id"], "team/Led")

    def test_source_files_are_created_and_extended(self):
        # 以前 create-index、add-index、add-source 成功时什么也不输出。
        # create-index, add-index and add-source used to print nothing on success.
        work = self.tmp / "work"
        work.mkdir()
        out, _ = self.ok("-C", work, "source", "create-index", cwd=work)
        self.assertEqual(out, "Created Modules/index.yaml with BlinkLED as an example entry\n")
        add_index = (
            "source",
            "add-index",
            "https://git.example.com/me/A.git",
            "--index",
            "Modules/index.yaml",
        )
        out, _ = self.ok(*add_index, cwd=work)
        self.assertEqual(out, "Added https://git.example.com/me/A.git to Modules/index.yaml\n")
        out, _ = self.ok(*add_index, cwd=work)
        self.assertEqual(
            out, "https://git.example.com/me/A.git is already listed in Modules/index.yaml\n"
        )
        self.assertEqual(
            yaml.safe_load(self.read(work / "Modules/index.yaml")),
            {
                "namespace": "local",
                "modules": [
                    "https://github.com/xrobot-org/BlinkLED.git",
                    "https://git.example.com/me/A.git",
                ],
                "bsps": [],
            },
        )
        sources = work / "x/sources.yaml"
        sources.parent.mkdir()
        self.ok("source", "--sources", sources, "create-sources", cwd=work)
        add_source = ("source", "--sources", sources, "add-source", "../Modules/index.yaml")
        out, _ = self.ok(*add_source, "--priority", "1")
        self.assertEqual(out, f"Added ../Modules/index.yaml to {sources}\n")
        out, _ = self.ok(*add_source)
        self.assertEqual(out, f"../Modules/index.yaml is already listed in {sources}\n")
        out, _ = self.ok("source", "add-source", "local/index.yaml")
        self.assertEqual(out, "Added local/index.yaml to Modules/sources.yaml\n")
        self.assertEqual(
            yaml.safe_load(self.read(sources)),
            {
                "sources": [
                    {"url": "https://xrobot.work/xrobot-modules/index.yaml", "priority": 0},
                    {"url": "../Modules/index.yaml", "priority": 1},
                ]
            },
        )

    def test_source_finds_the_bsp_like_other_commands(self):
        out, _ = self.ok("source", "search", "led", cwd=self.root / "User")
        self.assertIn("team/Led [module]", out)
        out, _ = self.ok("-C", self.root / "User", "source", "list", cwd=self.tmp)
        self.assertIn("team/Led [module]", out)

    def test_source_options_are_passed_through(self):
        out, _ = self.ok(
            "source", "--sources", self.root / "Modules/sources.yaml", "list", cwd=self.tmp
        )
        self.assertIn("team/Led [module]", out)
