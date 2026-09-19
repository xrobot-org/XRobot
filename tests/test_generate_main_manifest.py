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


def test_generated_main_is_parser_backed_without_changing_layout() -> None:
    from xrobot.GenerateMain import generate_xrobot_main_code

    config = {
        "global_settings": {"monitor_sleep_ms": 250},
        "modules": [
            {
                "id": "BlinkLED_0",
                "name": "BlinkLED",
                "constructor_args": {"blink_cycle": 500},
            }
        ],
    }
    generated = generate_xrobot_main_code("hw", ["BlinkLED"], config)
    assert generated == (
        '#include "app_framework.hpp"\n'
        '#include "libxr.hpp"\n'
        '\n'
        '// Module headers\n'
        '#include "BlinkLED.hpp"\n'
        '\n'
        'static void XRobotMain(LibXR::HardwareContainer &hw) {\n'
        '  using namespace LibXR;\n'
        '  ApplicationManager appmgr;\n'
        '\n'
        '  // Auto-generated module instantiations\n'
        '  static BlinkLED BlinkLED_0(hw, appmgr, 500);\n'
        '\n'
        '  while (true) {\n'
        '    appmgr.MonitorAll();\n'
        '    Thread::Sleep(250);\n'
        '  }\n'
        '}'
    )
