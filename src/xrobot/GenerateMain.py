import yaml
import argparse
from pathlib import Path
from collections import OrderedDict
from typing import Union, Dict, List


def parse_manifest_from_header(header_path: Path) -> Dict:
    """Extract and parse manifest data from module header file."""
    content = header_path.read_text(encoding="utf-8")
    manifest_block = []
    in_manifest = False

    for line in content.splitlines():
        stripped = line.strip()
        if stripped == "/* === MODULE MANIFEST ===":
            in_manifest = True
            continue
        if stripped == "=== END MANIFEST === */":
            break
        if in_manifest:
            cleaned_line = line.replace("/*", "  ").replace("*/", "  ").rstrip()
            manifest_block.append(cleaned_line)

    try:
        manifest_data = yaml.safe_load("\n".join(manifest_block)) or {}
    except yaml.YAMLError as e:
        print(f"[ERROR] YAML parsing failed: {str(e)}")
        return {}

    if 'constructor_args' in manifest_data:
        raw_args = manifest_data['constructor_args']
        ordered_args = OrderedDict()

        if isinstance(raw_args, dict):
            ordered_args.update(raw_args)
        elif isinstance(raw_args, list):
            for item in raw_args:
                if isinstance(item, dict):
                    ordered_args.update(item)
        else:
            print(f"[WARN] Unsupported constructor_args type: {type(raw_args)}")

        manifest_data['constructor_args'] = ordered_args

    return manifest_data


def _format_cpp_value(value: Union[dict, list, str, int, float, bool]) -> str:
    """Generate C++-compliant parameter values."""
    if isinstance(value, dict):
        inner = ", ".join(_format_cpp_value(v) for _, v in value.items())
        return f"{{{inner}}}"
    elif isinstance(value, list):
        return "{" + ", ".join(_format_cpp_value(v) for v in value) + "}"
    elif isinstance(value, bool):
        return "true" if value else "false"
    elif isinstance(value, str):
        return f'"{value}"'
    else:
        return str(value)


def extract_constructor_args(modules: List[str], module_dir: Path, config_path: Path) -> Dict:
    """Extract default arguments from modules and save to YAML config."""
    output = {
        "global_settings": {
            "monitor_sleep_ms": 1000  # 可根据需要设置默认值
        },
        "modules": {}
    }
    for mod in modules:
        hpp_path = module_dir / mod / f"{mod}.hpp"
        if not hpp_path.exists():
            continue

        manifest = parse_manifest_from_header(hpp_path)
        args_raw = manifest.get("constructor_args", {})
        args_ordered = OrderedDict()

        if isinstance(args_raw, (dict, list)):
            args_ordered.update(
                args_raw if isinstance(args_raw, dict) else {k: v for item in args_raw for k, v in item.items()})

        output["modules"][mod] = {"constructor_args": dict(args_ordered)}

    config_path.write_text(
        yaml.dump(output, sort_keys=False, allow_unicode=True, indent=2),
        encoding="utf-8"
    )
    return output


def generate_xrobot_main_code(hw_var: str, modules: List[str], config: Dict) -> str:
    """Generate main application code with module instantiations."""
    sleep_ms = config.get("global_settings", {}).get("monitor_sleep_ms", 1000)
    headers = [
                  '#include "app_framework.hpp"',
                  '#include "libxr.hpp"',
                  "",
                  "// Module headers"
              ] + [f'#include "{mod}.hpp"' for mod in modules]

    body = [
        f"template <typename HardwareContainer>",
        f"static void XRobotMain(HardwareContainer &{hw_var}) {{",
        f"  using namespace LibXR;",
        f"  ApplicationManager appmgr;",
        f"",
        f"  // Auto-generated module instantiations",
    ]

    for mod in modules:
        args_dict = config.get("modules", {}).get(mod, {}).get("constructor_args", {})
        args_list = [_format_cpp_value(v) for _, v in args_dict.items()]

        instance_line = f"  {mod}<HardwareContainer> {mod.lower()}({hw_var}, appmgr"
        if args_list:
            instance_line += ", " + ", ".join(args_list)
        instance_line += ");"
        body.append(instance_line)

    body += [
        "",
        f"  while (true) {{",
        f"    appmgr.MonitorAll();",
        f"    Thread::Sleep({sleep_ms});",
        f"  }}",
        f"}}"
    ]

    return "\n".join(headers + [""] + body)


def auto_discover_modules(modules_dir: Path = Path("Modules")) -> List[str]:
    """Discover available modules in directory."""
    return [sub.name for sub in modules_dir.iterdir() if sub.is_dir() and (sub / f"{sub.name}.hpp").exists()]


def main():
    parser = argparse.ArgumentParser(description="XRobot code generation tool")
    parser.add_argument("-o", "--output", required=True, help="Output CPP file path")
    parser.add_argument("-m", "--modules", nargs="+", default=[], help="List of modules to include")
    parser.add_argument("--hw", default="hw", help="Hardware container variable name")
    parser.add_argument("-c", "--config", help="Configuration file path")

    args = parser.parse_args()

    # Module discovery
    if not args.modules:
        args.modules = auto_discover_modules()
        print(f"Discovered modules: {', '.join(args.modules) or 'None'}")

    # Configuration handling
    config_data = {}
    if args.config:
        config_path = Path(args.config)
        if config_path.exists():
            config_data = yaml.safe_load(config_path.read_text(encoding="utf-8")) or {}
        else:
            config_data = extract_constructor_args(args.modules, Path("Modules"), config_path)

    # Code generation
    output_code = generate_xrobot_main_code(args.hw, args.modules, config_data)
    Path(args.output).write_text(output_code, encoding="utf-8")
    print(f"[SUCCESS] Generated entry file: {args.output}")


if __name__ == "__main__":
    main()