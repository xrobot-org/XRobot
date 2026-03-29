#!/usr/bin/env python3
"""
xrobot_codegen.py - Generate C++ main file and configuration for XRobot modules.

- Parses module manifest from header files.
- Extracts constructor/template arguments for each module.
- Auto-generates YAML config if not present.
- Generates C++ main application code for all modules.

Usage example:
  python xrobot_codegen.py -m BlinkLED Motor -o User/xrobot_main.hpp
  python xrobot_codegen.py               # Auto-discover modules, generate config and main
"""

import re
import yaml
import argparse
from pathlib import Path
from collections import OrderedDict
from typing import Union, Dict, List
from yaml.representer import SafeRepresenter

yaml.add_representer(OrderedDict, SafeRepresenter.represent_dict)

def parse_manifest_from_header(header_path: Path) -> Dict:
    """
    Extract and parse manifest data from the module header file.
    Supports V1/V2 manifest format auto-detection.
    """
    content = header_path.read_text(encoding="utf-8")
    manifest_block = []
    in_manifest = False

    for line in content.splitlines():
        stripped = line.strip()
        if "=== MODULE MANIFEST" in stripped:
            in_manifest = True
            continue
        if "=== END MANIFEST" in stripped:
            break
        if in_manifest:
            manifest_block.append(line.strip())

    if not manifest_block:
        print(f"[WARN] No manifest found in {header_path}")
        return {}

    try:
        manifest_data = yaml.safe_load("\n".join(manifest_block)) or {}
    except yaml.YAMLError as e:
        print(f"[ERROR] YAML parsing failed in {header_path}: {str(e)}")
        return {}

    # Normalize constructor_args and template_args as List[Dict]
    for key in ["constructor_args", "template_args"]:
        val = manifest_data.get(key)
        if isinstance(val, dict):
            manifest_data[key] = [{k: v} for k, v in val.items()]
        elif isinstance(val, list):
            manifest_data[key] = [
                {k: v} if isinstance(item, dict) else {str(item): ""}
                for item in val
                for k, v in (item.items() if isinstance(item, dict) else [(item, "")])
            ]
        elif val is None:
            manifest_data[key] = []
        elif isinstance(val, str):
            manifest_data[key] = [{val: ""}]
        else:
            print(f"[WARN] {key} format not recognized: {type(val)}")

    print(f"[INFO] Successfully parsed manifest for {header_path.stem}")
    return manifest_data

def _format_cpp_value(value: Union[dict, list, str, int, float, bool], key: str = "") -> str:
    """
    Format a value as C++-compliant parameter.
    Supports numbers, bool, identifiers, @instance, string, nested dict as {a,b,c}, and list as {a,b,c}.
    """
    if isinstance(value, dict):
        # 输出为聚合初始化列表，顺序由 yaml/OrderedDict 决定
        return '{' + ', '.join(_format_cpp_value(v) for v in value.values()) + '}'
    elif isinstance(value, list):
        return '{' + ', '.join(_format_cpp_value(v) for v in value) + '}'
    elif isinstance(value, bool):
        return "true" if value else "false"
    elif isinstance(value, str):
        # @instance 变量
        if value.startswith('@'):
            return value[1:]
        # C++ 枚举、作用域名等
        if re.match(r"^[A-Za-z_][\w:<>\s,]*::[A-Za-z_][\w:]*$", value):
            return value
        # 纯数字
        elif re.match(r"^-?\d+(\.\d+)?$", value):
            return value
        # 普通字符串
        else:
            return f'"{value}"'
    else:
        return str(value)

def extract_constructor_args(
    modules: List[str], module_dir: Path, config_path: Path
) -> Dict:
    """
    Extract default constructor_args/template_args for each module
    and save them to a YAML config file in a list format.
    """
    output = {
        "global_settings": {
            "monitor_sleep_ms": 1000
        },
        "modules": []
    }

    for mod in modules:
        hpp_path = module_dir / mod / f"{mod}.hpp"
        if not hpp_path.exists():
            print(f"[WARN] Header not found for module: {mod}")
            continue

        manifest = parse_manifest_from_header(hpp_path)
        if not manifest:
            print(f"[ERROR] Failed to parse manifest for {mod}")
            continue

        # Constructor args
        args_list = manifest.get("constructor_args", [])
        if isinstance(args_list, dict):
            args_list = [{k: v} for k, v in args_list.items()]
        elif isinstance(args_list, list):
            normalized = []
            for item in args_list:
                if isinstance(item, dict):
                    normalized.append(item)
                elif isinstance(item, str):
                    normalized.append({item: ""})
            args_list = normalized
        else:
            args_list = []

        args_ordered = OrderedDict()
        for d in args_list:
            if isinstance(d, dict):
                args_ordered.update(d)

        # Template args
        tmpl_list = manifest.get("template_args", [])
        if isinstance(tmpl_list, dict):
            tmpl_list = [{k: v} for k, v in tmpl_list.items()]
        elif isinstance(tmpl_list, list):
            normalized = []
            for item in tmpl_list:
                if isinstance(item, dict):
                    normalized.append(item)
                elif isinstance(item, str):
                    normalized.append({item: ""})
            tmpl_list = normalized
        else:
            tmpl_list = []

        tmpl_ordered = OrderedDict()
        for d in tmpl_list:
            if isinstance(d, dict):
                tmpl_ordered.update(d)

        # Generate config entry for module
        mod_entry = OrderedDict([
            ("name", mod),
            ("constructor_args", args_ordered)
        ])
        if tmpl_ordered:
            mod_entry["template_args"] = tmpl_ordered

        output["modules"].append(mod_entry)

    print(f"[INFO] Writing configuration to {config_path}")
    config_path.parent.mkdir(parents=True, exist_ok=True)
    config_path.write_text(
        yaml.dump(output, sort_keys=False, allow_unicode=True, indent=2),
        encoding="utf-8"
    )

    return output

def generate_xrobot_main_code(hw_var: str, modules: List[str], config: Dict) -> str:
    """
    Generate the main application code (C++ entry point) with module instantiations,
    supporting template args and instance id.
    """
    sleep_ms = config.get("global_settings", {}).get("monitor_sleep_ms", 1000)
    headers = [
        '#include "app_framework.hpp"',
        '#include "libxr.hpp"',
        "",
        "// Module headers"
    ] + [f'#include "{mod}.hpp"' for mod in modules]

    body = [
        f"static void XRobotMain(LibXR::HardwareContainer &{hw_var}) {{",
        f"  using namespace LibXR;",
        f"  ApplicationManager appmgr;",
        f"",
        f"  // Auto-generated module instantiations",
    ]

    module_entries = config.get("modules", [])
    if not isinstance(module_entries, list):
        raise TypeError("[ERROR] 'modules' must be a list of module instances")

    # Track auto-assigned instance names per module
    auto_inst_index = {}

    for entry in module_entries:
        mod = entry.get("name")
        inst_id = entry.get("id")
        if inst_id:
            instance_name = inst_id
        else:
            idx = auto_inst_index.get(mod, 0)
            instance_name = f"{mod.lower()}{idx}" if idx > 0 else mod.lower()
            auto_inst_index[mod] = idx + 1

        if mod not in modules:
            print(f"[WARN] Module {mod} not included in the provided list.")
            continue

        args_dict = entry.get("constructor_args", {})
        if isinstance(args_dict, dict):
            args_list = [_format_cpp_value(v, k) for k, v in args_dict.items()]
        else:
            args_list = []

        tmpl_dict = entry.get("template_args", {})
        if isinstance(tmpl_dict, dict) and tmpl_dict:
            tmpl_params = [str(v) for k, v in tmpl_dict.items()]
            tmpl_str = "<" + ", ".join(tmpl_params) + ">"
        else:
            tmpl_str = ""

        instance_line = f"  static {mod}{tmpl_str} {instance_name}({hw_var}, appmgr"
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
    """
    Discover available modules in the Modules directory.
    Returns a list of module names that have their .hpp files.
    """
    discovered_modules = [
        sub.name for sub in modules_dir.iterdir()
        if sub.is_dir() and (sub / f"{sub.name}.hpp").exists()
    ]
    if not discovered_modules:
        print("[WARN] No valid modules found in the Modules directory.")
    return discovered_modules

def extract_modules_from_config(config: Dict) -> List[str]:
    """
    Extract a de-duplicated module include list from config["modules"] while
    preserving declaration order.
    """
    module_entries = config.get("modules", [])
    if not isinstance(module_entries, list):
        print("[WARN] Config field 'modules' is not a list; falling back to discovery.")
        return []

    selected_modules = []
    seen_modules = set()
    for entry in module_entries:
        if not isinstance(entry, dict):
            continue

        mod = entry.get("name")
        if not isinstance(mod, str):
            continue

        mod = mod.strip()
        if not mod or mod in seen_modules:
            continue

        selected_modules.append(mod)
        seen_modules.add(mod)

    return selected_modules

def main():
    parser = argparse.ArgumentParser(description="XRobot code generation tool")
    parser.add_argument("-o", "--output", default='User/xrobot_main.hpp', help="Output C++ file path")
    parser.add_argument("-m", "--modules", nargs="+", default=[], help="List of modules to include")
    parser.add_argument("--hw", default="hw", help="Hardware container variable name")
    parser.add_argument("-c", "--config", help="Configuration YAML file path")

    args = parser.parse_args()

    # Configuration handling
    config_data = {}
    config_path = Path(args.config) if args.config else Path("User/xrobot.yaml")
    if config_path.exists():
        print(f"[INFO] Using existing configuration file: {config_path}")
        config_data = yaml.safe_load(config_path.read_text(encoding="utf-8")) or {}
    elif args.config:
        print(f"[WARN] Configuration file not found: {config_path}")

    # Module selection
    if not args.modules:
        args.modules = extract_modules_from_config(config_data)
        if args.modules:
            print(f"[INFO] Using modules from configuration: {', '.join(args.modules)}")
            modules_dir = Path("Modules")
            for mod in args.modules:
                hpp = modules_dir / mod / f"{mod}.hpp"
                if not hpp.exists():
                    print(f"[WARN] Module '{mod}' declared in config but header not found: {hpp}")
        else:
            args.modules = auto_discover_modules()
            print(f"Discovered modules: {', '.join(args.modules) or 'None'}")

    if not config_path.exists():
        config_data = extract_constructor_args(args.modules, Path("Modules"), config_path)

    # Code generation
    output_code = generate_xrobot_main_code(args.hw, args.modules, config_data)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(output_code, encoding="utf-8")
    print(f"[SUCCESS] Generated entry file: {args.output}")

if __name__ == "__main__":
    main()
