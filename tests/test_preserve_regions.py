"""Regression tests for user ownership across regeneration."""
import unittest
import tempfile
import contextlib
import io
from pathlib import Path
import yaml
from xrobot.GenerateMain import generate
from xrobot.ModuleCreator import create_module

from xrobot.SourceSyntax import preserve_regions


class PreserveRegionsTest(unittest.TestCase):
    def test_preserves_user_body_and_refreshes_generated_protected_code(self):
        existing = """#pragma once
/* User Code Begin XRobotMain */
custom_user_hook();
 /* User Code End XRobotMain */
// clang-format off
old_macro_layout()
// clang-format on
// NOLINTBEGIN
old_generated_assert()
// NOLINTEND
"""
        generated = """#pragma once
int regenerated_prefix;
/* User Code Begin XRobotMain */
generated_user_hook();
/* User Code End XRobotMain */
// clang-format off
new_macro_layout()
// clang-format on
// NOLINTBEGIN
new_generated_assert()
// NOLINTEND
int regenerated_suffix;
"""
        result = preserve_regions(existing, generated)
        self.assertIn("int regenerated_prefix;", result)
        self.assertIn("int regenerated_suffix;", result)
        self.assertIn("custom_user_hook();", result)
        self.assertNotIn("generated_user_hook();", result)
        self.assertNotIn("old_macro_layout()", result)
        self.assertIn("new_macro_layout()", result)
        self.assertNotIn("old_generated_assert()", result)
        self.assertIn("new_generated_assert()", result)

    def test_regeneration_changes_registered_argument(self):
        """Switch a consumed registration without freezing the old call macro."""
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            modules = root / 'Modules'
            create_module('Foo', constructor_args=['int& value'], output_dir=modules)
            source = root / 'app_main.cpp'
            source.write_text('XR_REGISTER(left, int);\nXR_REGISTER(right, int);\n')
            config = root / 'xrobot.yaml'
            output = root / 'xrobot_main.hpp'
            for name in ('left', 'right', 'left'):
                config.write_text(yaml.safe_dump({'modules': [
                    {'module': 'Foo', 'id': 'foo', 'args': [{'value': name}]}
                ]}))
                with contextlib.redirect_stdout(io.StringIO()):
                    actual = generate(config, modules, output, [source])
                    clean = generate(config, modules, root / (name + '.hpp'), [source])
                self.assertEqual(actual, clean)
                self.assertIn('#define XROBOT_MAIN() ::XRobotMain(' + name + ')', actual)

    def test_nested_protection_stays_inside_user_code(self):
        """Keep exact user-owned contents even when format/lint markers nest."""
        body = '\n// clang-format off\n// NOLINTBEGIN\ncustom();\n// NOLINTEND\n// clang-format on\n'
        old = '/* User Code Begin XRobotMain */' + body + '/* User Code End XRobotMain */'
        new = '/* User Code Begin XRobotMain */\n/* User Code End XRobotMain */'
        self.assertEqual(preserve_regions(old, new), old)


if __name__ == "__main__":
    unittest.main()
