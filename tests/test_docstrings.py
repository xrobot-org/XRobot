"""Every module, class and function of the package, tools/ and scripts/ has a Chinese and
English docstring (ported from xr-syntax's test_public_docstrings)."""

import ast
import re
import unittest
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[1]
CJK = re.compile(r"[㐀-鿿]")
LATIN_WORD = re.compile(r"[A-Za-z]{3,}")


def checked_files() -> list[Path]:
    return [
        path
        for folder in ("src/xrobot", "tools", "scripts")
        for path in sorted((REPOSITORY / folder).glob("*.py"))
    ]


def bilingual(doc: str) -> bool:
    """Chinese somewhere, and at least one line of English without Chinese."""
    return bool(CJK.search(doc)) and any(
        LATIN_WORD.search(line) and not CJK.search(line) for line in doc.splitlines()
    )


def definitions_without_bilingual_docstrings() -> list[str]:
    missing = []
    for path in checked_files():
        tree = ast.parse(path.read_text(encoding="utf-8"))
        nodes = [(tree, "<module>", 1)] + [
            (node, node.name, node.lineno)
            for node in ast.walk(tree)
            if isinstance(node, (ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef))
        ]
        for node, name, line in nodes:
            if not bilingual(ast.get_docstring(node) or ""):
                missing.append(f"{path.relative_to(REPOSITORY).as_posix()}:{line} {name}")
    return missing


class Docstrings(unittest.TestCase):
    def test_every_definition_has_a_chinese_and_english_docstring(self):
        self.assertEqual(definitions_without_bilingual_docstrings(), [])

    def test_the_check_tells_one_language_from_two(self):
        self.assertTrue(bilingual("说明。\nWhat it does."))
        self.assertFalse(bilingual("What it does."))
        self.assertFalse(bilingual("说明 OnMonitor 的调用。"))
