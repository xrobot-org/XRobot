"""SourceSyntax 构造接口提取的条件编译合同。"""
import unittest

from xrobot.SourceSyntax import extract_interface


class ConditionalConstructorTest(unittest.TestCase):
    def test_constructor_under_conditional_is_rejected(self):
        """构造接口随 #if/#ifdef/#ifndef 变化时必须拒绝。"""
        for directive in ("#if FEATURE", "#ifdef FEATURE", "#ifndef FEATURE"):
            source = "class Foo {\n public:\n%s\n  Foo(int value);\n#endif\n};" % directive
            with self.subTest(directive=directive):
                with self.assertRaises(ValueError):
                    extract_interface(source, "Foo")

    def test_closed_conditional_before_constructor_is_accepted(self):
        """构造函数前已闭合的条件块不影响接口提取。"""
        source = """class Foo {
 public:
#if FEATURE
  int extra_;
#elif OTHER
  int other_;
#else
  int none_;
#endif  // FEATURE
  Foo(int value);
};"""
        interface = extract_interface(source, "Foo")
        self.assertEqual(["int value"], interface["constructors"][0]["parameters"])


if __name__ == "__main__":
    unittest.main()
