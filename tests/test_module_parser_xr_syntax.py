"""Regression tests for xr-syntax-backed XRobot module manifest parsing."""

from pathlib import Path

from xrobot.ModuleParser import parse_constructor_args, parse_manifest_from_header


def _write_header(tmp_path: Path, source: str, name: str = "Demo") -> Path:
    path = tmp_path / f"{name}.hpp"
    path.write_text(source, encoding="utf-8")
    return path


def test_v2_manifest_is_read_from_cpp_comment(tmp_path: Path) -> None:
    header = _write_header(
        tmp_path,
        """#pragma once
// unrelated comment
/* === MODULE MANIFEST V2 ===
module_description: demo
constructor_args:
  - period: 250
template_args:
  - T: int
required_hardware: led
depends: []
=== END MANIFEST === */
class Demo {};
""",
    )
    manifest = parse_manifest_from_header(header)
    assert manifest is not None
    assert manifest.description == "demo"
    assert list(parse_constructor_args(manifest.constructor_args).items()) == [
        ("period", 250)
    ]
    assert list(parse_constructor_args(manifest.template_args).items()) == [
        ("T", "int")
    ]
    assert manifest.required_hardware == ["led"]


def test_v1_and_case_insensitive_markers_remain_supported(tmp_path: Path) -> None:
    header = _write_header(
        tmp_path,
        """/* === module manifest ===
module_description: lower
constructor_args: speed
=== end manifest === */
class Demo {};
""",
    )
    manifest = parse_manifest_from_header(header)
    assert manifest is not None
    assert manifest.description == "lower"
    assert list(parse_constructor_args(manifest.constructor_args)) == ["speed"]


def test_manifest_can_share_a_single_block_comment_line(tmp_path: Path) -> None:
    header = _write_header(
        tmp_path,
        "/* === MODULE MANIFEST === module_description: inline "
        "=== END MANIFEST === */\nclass Demo {};\n",
    )
    manifest = parse_manifest_from_header(header)
    assert manifest is not None
    assert manifest.description == "inline"


def test_unrelated_comments_do_not_form_a_manifest(tmp_path: Path) -> None:
    header = _write_header(
        tmp_path,
        "/* MODULE MANIFEST mentioned in prose only */\nclass Demo {};\n",
    )
    assert parse_manifest_from_header(header) is None


def test_utf8_bom_header_is_supported(tmp_path: Path) -> None:
    path = tmp_path / "Demo.hpp"
    path.write_bytes(
        b"\xef\xbb\xbf/* === MODULE MANIFEST V2 ===\n"
        b"module_description: bom\n"
        b"constructor_args: []\n"
        b"=== END MANIFEST === */\n"
        b"class Demo {};\n"
    )
    manifest = parse_manifest_from_header(path)
    assert manifest is not None
    assert manifest.description == "bom"
