"""锁定旧 CppSource 与 xr-syntax consumer 迁移期间的 golden parity。

这些测试只用于迁移期。生产代码不得重新依赖 CppSource；当真实生态 parity
完成并删除旧实现时，本文件也应随兼容基线一起收掉。
"""

from __future__ import annotations

import re
import tempfile
import unittest
from pathlib import Path

from xrobot.ConstructorModel import enrich_interface
from xrobot.CppSource import (
    bind_identifiers as legacy_bind_identifiers,
    close_token as legacy_close_token,
    code_tokens as legacy_code_tokens,
    extract_interface as legacy_extract_interface,
    split_arguments as legacy_split_arguments,
)
from xrobot.GenerateMain import (
    caller_defined_names,
    caller_scoped_view,
    read_registrations,
)
from xrobot.SourceSyntax import (
    bind_identifiers,
    code_tokens,
    extract_interface,
    split_arguments,
)


def _interface_snapshot(source: str, name: str = "Foo") -> dict:
    """返回新 parser 的 XRobot 构造接口语义快照。"""
    return _normalized_interface(
        enrich_interface(source, extract_interface(source, name))
    )


def _legacy_interface_snapshot(source: str, name: str = "Foo") -> dict:
    """返回旧 CppSource parser 的同形语义快照。"""
    return _normalized_interface(
        enrich_interface(source, legacy_extract_interface(source, name))
    )


def _normalized_interface(interface: dict) -> dict:
    """去掉仅用于展示的 declaration 文本，只比较 consumer 真正读取的语义。"""
    return {
        "name": interface["name"],
        "template_parameters": interface["template_parameters"],
        "symbols": interface["symbols"],
        "aliases": interface["aliases"],
        "constructors": [
            {
                "line": constructor["line"],
                "arguments": constructor["arguments"],
            }
            for constructor in interface["constructors"]
        ],
    }


def _legacy_read_registrations(path: Path) -> list[dict]:
    """保留迁移前 read_registrations 算法，作为 XR_REGISTER golden oracle。"""
    text = path.read_text(encoding="utf-8-sig")
    items = legacy_code_tokens(text)
    records = []
    names = set()
    for index, token in enumerate(items):
        if (
            token.text != "XR_REGISTER"
            or index + 1 >= len(items)
            or items[index + 1].text != "("
        ):
            continue
        end = legacy_close_token(items, index + 1)
        parts = legacy_split_arguments(
            text[items[index + 1].end : items[end].start]
        )
        if len(parts) < 2 or not re.fullmatch(
            r"[A-Za-z_][A-Za-z_0-9]*", parts[0]
        ):
            raise ValueError(
                "%s: XR_REGISTER requires an existing name and explicit object types"
                % path
            )
        name = parts[0]
        if name in names:
            raise ValueError("Duplicate XR_REGISTER name: " + name)
        if name.startswith("xr_"):
            raise ValueError(
                "Registration collides with generated identifier prefix: " + name
            )
        if len(set(parts[1:])) != len(parts[1:]):
            raise ValueError("Duplicate view in XR_REGISTER: " + name)
        if any(part.rstrip().endswith("&") for part in parts[1:]):
            raise ValueError("Register object types, not reference types: " + name)
        names.add(name)
        local_names = caller_defined_names(items, index)
        records.append(
            {
                "name": name,
                "types": parts[1:],
                "source": str(path),
                "line": text.count("\n", 0, token.start) + 1,
                "caller_views": [
                    cpp_type
                    for cpp_type in parts[1:]
                    if caller_scoped_view(cpp_type, local_names)
                ],
            }
        )
    return records


class SourceSyntaxGoldenParity(unittest.TestCase):
    """比较旧 reader 与新 xr-syntax reader 的 XRobot 可观测行为。"""

    def test_constructor_interface_matches_legacy_reader(self):
        """覆盖模板、重载、别名、嵌套类型和 deleted 特殊成员。"""
        sources = [
            """template <typename T = std::array<int, 2>, int N = 4>
class Foo {
 public:
  using Value = T;
  enum class Mode { A, B };
  Foo(Value value = Value{}, Mode mode = Mode::A) {}
  Foo(int count, const char* text = "x") {}
  Foo(const Foo&) = delete;
 private:
  Foo(double hidden);
};""",
            """struct Foo {
  typedef struct { int count; } Param;
  Foo(const Param& param = Param{.count = 3});
  Foo(float gain = 1.0f);
};""",
            """class Foo {
 public:
  static int Factory();
  Foo(int value = Factory());
  Foo& operator=(const Foo&) = delete;
  ~Foo() = default;
};""",
        ]
        for source in sources:
            with self.subTest(source=source):
                self.assertEqual(
                    _interface_snapshot(source),
                    _legacy_interface_snapshot(source),
                )

    def test_lexical_binding_matches_legacy_reader(self):
        """覆盖 Unicode、注释、raw string、成员名和作用域限定符。"""
        expressions = [
            'f(dev, &dev, dev.member, obj.dev, ptr->dev, ns::dev, '
            'dev::constant, dev2, "dev", R"x(dev)x", /* 中文 dev */ dev)',
            'std::array<int, 2>{dev, 1} + Vendor::dev + (dev)',
        ]
        bindings = {"dev": ["xr_view_dev_0"]}
        for expression in expressions:
            with self.subTest(expression=expression):
                self.assertEqual(
                    bind_identifiers(expression, bindings),
                    legacy_bind_identifiers(expression, bindings),
                )

    def test_argument_splitting_matches_legacy_reader(self):
        """覆盖模板逗号、brace initializer、lambda 和字符串中的逗号。"""
        values = [
            'std::array<int, 2>{1, 2}, Foo{.x = 1, .y = 2}, "a,b"',
            'T value = T{1, 2}, const char* text = "),", int n = 3',
            'Callback{[](int a, int b) { return a + b; }}, 7',
        ]
        for value in values:
            with self.subTest(value=value):
                self.assertEqual(
                    split_arguments(value),
                    legacy_split_arguments(value),
                )

    def test_public_code_tokens_match_legacy_contract(self):
        """比较 ConstructorModel 依赖的 token 文本、位置和公共 kind。"""
        source = 'const Foo<T, 2>* value = Make(1e-3, "x");'
        self.assertEqual(
            [(item.text, item.start, item.end, item.kind) for item in code_tokens(source)],
            [
                (item.text, item.start, item.end, item.kind)
                for item in legacy_code_tokens(source)
            ],
        )

    def test_xr_register_reader_matches_legacy_algorithm(self):
        """覆盖全局/局部 view、模板类型、函数指针 alias 和多条注册。"""
        source = """#include "xrobot_main.hpp"
using GlobalView = LibXR::GPIO;
static GlobalView global_gpio;
XR_REGISTER(global_gpio, GlobalView);

int main() {
  constexpr unsigned Count{2};
  using LocalView = std::array<int, Count>;
  static LocalView values{};
  typedef void (*LocalCallback)(int);
  static LocalCallback callback = nullptr;
  XR_REGISTER(values, LocalView);
  XR_REGISTER(callback, LocalCallback);
  return 0;
}
"""
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "app_main.cpp"
            path.write_text(source, encoding="utf-8")
            self.assertEqual(
                read_registrations([path]),
                _legacy_read_registrations(path),
            )

    def test_preprocessor_conditional_interface_stays_rejected(self):
        """旧 reader 拒绝条件化构造接口，新 reader 必须保持同一合同。"""
        source = """class Foo {
 public:
#if FEATURE
  Foo(int value);
#endif
};"""
        with self.assertRaises(ValueError):
            extract_interface(source, "Foo")
        with self.assertRaises(ValueError):
            legacy_extract_interface(source, "Foo")


if __name__ == "__main__":
    unittest.main()
