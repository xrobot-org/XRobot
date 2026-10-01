"""包、tools/ 和 tests/ 中每个模块、类和函数都有中英 docstring（移植自 xr-syntax）。
Every module, class and function of the package, tools/ and tests/ has a Chinese and English
docstring (ported from xr-syntax).

测试方法和 unittest 钩子除外：测试名说明它检查什么。
Test methods and unittest hooks are exempt: the test name says what it checks.
"""

import ast
import re
from pathlib import Path

from fixtures import TestCase

REPOSITORY = Path(__file__).resolve().parents[1]
CJK = re.compile(r"[㐀-鿿]")
LATIN_WORD = re.compile(r"[A-Za-z]{3,}")
TEST_HOOKS = {"setUp", "tearDown", "setUpClass", "tearDownClass"}


def checked_files() -> list[Path]:
    """要检查的源文件。
    The source files to check.
    """
    return [
        path
        for folder in ("src/xrobot", "tools", "tests")
        for path in sorted((REPOSITORY / folder).glob("*.py"))
    ]


def bilingual(doc: str) -> bool:
    """含有中文，并且至少有一行不含中文的英文。
    Chinese somewhere, and at least one line of English without Chinese.
    """
    return bool(CJK.search(doc)) and any(
        LATIN_WORD.search(line) and not CJK.search(line) for line in doc.splitlines()
    )


def definitions(tree: ast.Module, tests: bool) -> list[tuple[ast.AST, str, int]]:
    """需要 docstring 的模块和定义；tests 为真时跳过测试方法、钩子及其内部定义。
    The module and the definitions that need a docstring; with tests, test methods, hooks and
    what they contain are skipped.
    """
    found: list[tuple[ast.AST, str, int]] = [(tree, "<module>", 1)]
    pending: list[ast.AST] = [tree]
    while pending:
        for child in ast.iter_child_nodes(pending.pop()):
            if isinstance(child, (ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)):
                if tests and (child.name.startswith("test_") or child.name in TEST_HOOKS):
                    continue
                found.append((child, child.name, child.lineno))
            pending.append(child)
    return found


def definitions_without_bilingual_docstrings() -> list[str]:
    """缺少中英 docstring 的定义，形如 path:line name。
    The definitions without a bilingual docstring, as path:line name.
    """
    missing = []
    for path in checked_files():
        tree = ast.parse(path.read_text(encoding="utf-8"))
        for node, name, line in definitions(tree, path.parent.name == "tests"):
            if not bilingual(ast.get_docstring(node) or ""):
                missing.append(f"{path.relative_to(REPOSITORY).as_posix()}:{line} {name}")
    return missing


class Docstrings(TestCase):
    """docstring 检查。
    The docstring check.
    """

    def test_every_definition_has_a_chinese_and_english_docstring(self):
        self.assertEqual(definitions_without_bilingual_docstrings(), [])

    def test_test_methods_and_hooks_are_exempt_only_in_tests(self):
        tree = ast.parse(
            "class C:\n"
            "    def setUp(self): pass\n"
            "    def test_x(self):\n"
            "        def inner(): pass\n"
            "    def helper(self): pass\n"
        )
        self.assertEqual(
            [name for _, name, _ in definitions(tree, tests=True)], ["<module>", "C", "helper"]
        )
        self.assertEqual(
            sorted(name for _, name, _ in definitions(tree, tests=False)),
            ["<module>", "C", "helper", "inner", "setUp", "test_x"],
        )

    def test_the_check_tells_one_language_from_two(self):
        self.assertTrue(bilingual("说明。\nWhat it does."))
        self.assertFalse(bilingual("What it does."))
        self.assertFalse(bilingual("说明 OnMonitor 的调用。"))
