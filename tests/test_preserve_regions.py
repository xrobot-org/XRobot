import unittest

from xrobot.SourceSyntax import preserve_regions


class PreserveRegionsTest(unittest.TestCase):
    def test_preserves_user_format_and_lint_bodies(self):
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
        self.assertIn("old_macro_layout()", result)
        self.assertNotIn("new_macro_layout()", result)
        self.assertIn("old_generated_assert()", result)
        self.assertNotIn("new_generated_assert()", result)


if __name__ == "__main__":
    unittest.main()
