"""Tests for GenerateMain manifest normalization through ModuleParser."""

from pathlib import Path

from xrobot.GenerateMain import parse_manifest_from_header


def _write(tmp_path: Path, body: str) -> Path:
    path = tmp_path / "Demo.hpp"
    path.write_text(body, encoding="utf-8")
    return path


def test_generate_main_manifest_normalization_is_preserved(tmp_path: Path) -> None:
    header = _write(
        tmp_path,
        """/* === MODULE MANIFEST V2 ===
module_description: demo
constructor_args:
  period: 250
  mode: Fast
template_args:
  T: int
required_hardware: led
=== END MANIFEST === */
class Demo {};
""",
    )
    manifest = parse_manifest_from_header(header)
    assert manifest["constructor_args"] == [{"period": 250}, {"mode": "Fast"}]
    assert manifest["template_args"] == [{"T": "int"}]
    assert manifest["required_hardware"] == "led"


def test_generate_main_normalizes_string_and_list_arguments(tmp_path: Path) -> None:
    header = _write(
        tmp_path,
        """/* === MODULE MANIFEST ===
constructor_args:
  - speed
  - gain: 2.5
template_args: T
=== END MANIFEST === */
class Demo {};
""",
    )
    manifest = parse_manifest_from_header(header)
    assert manifest["constructor_args"] == [{"speed": ""}, {"gain": 2.5}]
    assert manifest["template_args"] == [{"T": ""}]


def test_generate_main_returns_empty_dict_without_manifest(tmp_path: Path) -> None:
    header = _write(tmp_path, "class Demo {};\n")
    assert parse_manifest_from_header(header) == {}
