"""Golden parity for the xr-syntax consumer migration."""

import tempfile
import unittest
from pathlib import Path

from xrobot.CppSource import extract_interface as legacy_extract_interface
from xrobot.GenerateMain import read_registrations as legacy_read_registrations
from xrobot.SourceSyntax import extract_interface, read_registrations


class SourceSyntaxParity(unittest.TestCase):
    def test_constructor_interface_matches_legacy_scanner(self):
        cases = [
            (
                "Foo",
                '// class Foo { Foo(int fake); };\n'
                'template <typename T = std::array<int, 2>, int N = 4>\n'
                'class Foo {\npublic:\n'
                ' explicit Foo(T value = T{1, 2}, const char* text = "),") : value_(value) {}\n'
                ' Foo(int n);\n Foo(const Foo&) = delete;\nprivate:\n'
                ' Foo(double);\n T value_;\n};',
            ),
            ("Foo", "struct Foo { Foo(int count=10); Foo(float gain=1.0f); };"),
            ("Foo", "class Foo { public: Foo(); Foo(int x) {} private: Foo(double); };"),
        ]
        for name, source in cases:
            with self.subTest(source=source):
                self.assertEqual(
                    extract_interface(source, name),
                    legacy_extract_interface(source, name),
                )

    def test_rejected_interfaces_match_legacy_contract(self):
        cases = (
            "namespace n { class Foo { public: Foo(); }; }",
            "class Foo { public: DEFINE_CTOR(Foo); };",
            "class Foo { public:\n#if FEATURE\nFoo(int);\n#endif\n};",
        )
        for source in cases:
            with self.subTest(source=source):
                with self.assertRaises(ValueError):
                    legacy_extract_interface(source, "Foo")
                with self.assertRaises(ValueError):
                    extract_interface(source, "Foo")

    def test_registration_call_structure_matches_legacy(self):
        source = '#include <array>\n'
        source += 'int main(){\n'
        source += '  XR_REGISTER(array, std::array<int, 2>);\n'
        source += '  // XR_REGISTER(fake, Wrong);\n'
        source += '  XR_REGISTER(device, const Device, Device*);\n'
        source += '}\n'
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "main.cpp"
            path.write_text(source, encoding="utf-8")
            legacy = legacy_read_registrations([path])
            migrated = read_registrations([path])
            keys = ("name", "types", "source", "line")
            self.assertEqual(
                [{key: record[key] for key in keys} for record in migrated],
                [{key: record[key] for key in keys} for record in legacy],
            )

    def test_registration_validation_matches_legacy(self):
        cases = (
            "XR_REGISTER(ref, Type&);",
            "XR_REGISTER(xr_bad, Type);",
            "XR_REGISTER(dev, Type, Type);",
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "main.cpp"
            for source in cases:
                with self.subTest(source=source):
                    path.write_text(source, encoding="utf-8")
                    with self.assertRaises(ValueError):
                        legacy_read_registrations([path])
                    with self.assertRaises(ValueError):
                        read_registrations([path])


if __name__ == "__main__":
    unittest.main()
