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
import json
import yaml
import argparse
from pathlib import Path
from collections import OrderedDict
from typing import Union, Dict, List, Optional
from yaml.representer import SafeRepresenter

yaml.add_representer(OrderedDict, SafeRepresenter.represent_dict)

DEFAULT_CONSTEXPR_NAMESPACE = "ProjectConstexpr"

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
            manifest_block.append(line)

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

def _validate_cpp_identifier(name: str, field_name: str = "identifier") -> str:
    if not isinstance(name, str) or not re.match(r"^[A-Za-z_]\w*$", name):
        raise ValueError(f"[ERROR] Invalid {field_name}: {name!r}")
    return name


def _validate_constexpr_ref(name: str) -> str:
    return _validate_cpp_identifier(name, "constexpr reference")


def _get_constexpr_namespace(config: Dict) -> str:
    return _validate_cpp_identifier(
        config.get("constexpr_namespace", DEFAULT_CONSTEXPR_NAMESPACE),
        "constexpr namespace",
    )


def _is_single_key_mapping(value: object, key: str) -> bool:
    return isinstance(value, dict) and set(value.keys()) == {key}


def _is_constexpr_ref(value: object) -> bool:
    return _is_single_key_mapping(value, "constexpr")


def _is_expr_value(value: object) -> bool:
    return _is_single_key_mapping(value, "expr")


def _is_string_value(value: object) -> bool:
    return _is_single_key_mapping(value, "string")


def _format_include_directive(header: str) -> str:
    header = header.strip()
    if not header:
        raise ValueError("[ERROR] include header must be a non-empty string")
    if (header.startswith("<") and header.endswith(">")) or (
        header.startswith('"') and header.endswith('"')
    ):
        return f"#include {header}"
    return f'#include "{header}"'


def _format_raw_expr(value: object) -> str:
    if isinstance(value, (dict, list)) or value is None:
        raise TypeError("[ERROR] 'expr' must be a scalar value")
    if isinstance(value, bool):
        return "true" if value else "false"
    expr = str(value).strip()
    if not expr:
        raise ValueError("[ERROR] 'expr' must be a non-empty scalar")
    return expr


def _format_string_literal(value: object) -> str:
    if isinstance(value, (dict, list)) or value is None:
        raise TypeError("[ERROR] 'string' must be a scalar value")
    if isinstance(value, bool):
        return json.dumps("true" if value else "false")
    return json.dumps(str(value))


def _format_cpp_value(
    value: Union[dict, list, str, int, float, bool],
    key: str = "",
    constexpr_namespace: str = DEFAULT_CONSTEXPR_NAMESPACE,
) -> str:
    """
    Format a value as C++-compliant parameter.
    Supports numbers, bool, identifiers, @instance, {constexpr: Name},
    {expr: RawExpr}, {string: Literal}, nested dict as {a,b,c}, and list as {a,b,c}.
    """
    if isinstance(value, dict):
        if _is_constexpr_ref(value):
            return f"{constexpr_namespace}::{_validate_constexpr_ref(value['constexpr'])}"
        if _is_expr_value(value):
            return _format_raw_expr(value["expr"])
        if _is_string_value(value):
            return _format_string_literal(value["string"])
        return '{' + ', '.join(
            _format_cpp_value(v, constexpr_namespace=constexpr_namespace)
            for v in value.values()
        ) + '}'
    if isinstance(value, list):
        return '{' + ', '.join(
            _format_cpp_value(v, constexpr_namespace=constexpr_namespace)
            for v in value
        ) + '}'
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, str):
        if value.startswith('@'):
            return value[1:]
        if re.match(r"^[A-Za-z_][\w:<>\s,]*::[A-Za-z_][\w:]*$", value):
            return value
        if re.match(r"^-?\d+(\.\d+)?$", value):
            return value
        return _format_string_literal(value)
    return str(value)


def _format_template_arg(
    value: Union[dict, list, str, int, float, bool],
    constexpr_namespace: str = DEFAULT_CONSTEXPR_NAMESPACE,
) -> str:
    """
    Format a template argument while preserving raw type-expression strings.
    """
    if _is_constexpr_ref(value):
        return f"{constexpr_namespace}::{_validate_constexpr_ref(value['constexpr'])}"
    if _is_expr_value(value):
        return _format_raw_expr(value["expr"])
    if _is_string_value(value):
        return _format_string_literal(value["string"])
    if isinstance(value, str):
        return value
    return _format_cpp_value(value, constexpr_namespace=constexpr_namespace)


def _format_multiline_template_type(mod: str, tmpl_params: List[str]) -> List[str]:
    lines = [f"{mod}<"]
    for idx, param in enumerate(tmpl_params):
        comma = "," if idx < len(tmpl_params) - 1 else ""
        lines.append(f"      {param}{comma}")
    lines.append("  >")
    return lines


def _format_module_instance_statement(
    mod: str,
    tmpl_params: List[str],
    instance_name: str,
    hw_var: str,
    args_list: List[str],
) -> str:
    call_args = [hw_var, "appmgr", *args_list]
    type_expr = mod
    if tmpl_params:
        type_expr += "<" + ", ".join(tmpl_params) + ">"

    single_line = f"  static {type_expr} {instance_name}(" + ", ".join(call_args) + ");"
    if len(single_line) <= 100:
        return single_line

    if tmpl_params and len(type_expr) > 48:
        type_lines = _format_multiline_template_type(mod, tmpl_params)
        lines = [f"  static {type_lines[0]}"]
        lines.extend(type_lines[1:-1])
        lines.append(f"{type_lines[-1]} {instance_name}(")
    else:
        lines = [f"  static {type_expr} {instance_name}("]

    for idx, arg in enumerate(call_args):
        comma = "," if idx < len(call_args) - 1 else ""
        lines.append(f"      {arg}{comma}")
    lines.append("  );")
    return "\n".join(lines)


def _generate_constexpr_header(config: Dict) -> Optional[str]:
    constexprs = config.get("constexprs", {})
    if not constexprs:
        return None
    if not isinstance(constexprs, dict):
        raise TypeError("[ERROR] 'constexprs' must be a mapping")

    include_headers = config.get("constexpr_includes", [])
    if include_headers is None:
        include_headers = []
    if not isinstance(include_headers, list) or not all(isinstance(h, str) for h in include_headers):
        raise TypeError("[ERROR] 'constexpr_includes' must be a list of header strings")

    constexpr_namespace = _get_constexpr_namespace(config)

    lines = ["#pragma once", ""]
    emitted_includes = []
    seen_includes = set()
    for header in include_headers:
        directive = _format_include_directive(header)
        if directive in seen_includes:
            continue
        seen_includes.add(directive)
        emitted_includes.append(directive)
    lines.extend(emitted_includes)
    if emitted_includes:
        lines.append("")
    lines.append(f"namespace {constexpr_namespace} {{")

    for name, spec in constexprs.items():
        _validate_constexpr_ref(name)
        if not isinstance(spec, dict):
            raise TypeError(f"[ERROR] constexpr '{name}' must be a mapping")
        cpp_type = spec.get("type")
        cpp_value = spec.get("value")
        if not isinstance(cpp_type, str) or not cpp_type:
            raise ValueError(f"[ERROR] constexpr '{name}' missing non-empty 'type'")
        if cpp_value is None:
            raise ValueError(f"[ERROR] constexpr '{name}' missing 'value'")
        lines.append(
            f"inline constexpr {cpp_type} {name} = "
            f"{_format_cpp_value(cpp_value, constexpr_namespace=constexpr_namespace)};"
        )

    lines += [
        f"}}  // namespace {constexpr_namespace}",
        "",
    ]
    return "\n".join(lines)


def _load_config_file(config_path: Path) -> Dict:
    try:
        config_data = yaml.safe_load(config_path.read_text(encoding="utf-8")) or {}
    except yaml.YAMLError as e:
        raise ValueError(f"[ERROR] Failed to parse YAML in {config_path}: {e}") from e
    if not isinstance(config_data, dict):
        raise TypeError(f"[ERROR] Top-level config in {config_path} must be a mapping")
    return config_data


def _require_mapping(value: object, field_name: str) -> Dict:
    if value is None:
        return {}
    if not isinstance(value, dict):
        raise TypeError(f"[ERROR] '{field_name}' must be a mapping")
    return value

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
    auto_inst_index = {}

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
        idx = auto_inst_index.get(mod, 0)
        instance_id = f"{mod}_{idx}"
        auto_inst_index[mod] = idx + 1
        mod_entry = OrderedDict([
            ("id", instance_id),
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
    if not isinstance(config, dict):
        raise TypeError("[ERROR] top-level config must be a mapping")

    global_settings = _require_mapping(config.get("global_settings", {}), "global_settings")
    sleep_ms = global_settings.get("monitor_sleep_ms", 1000)
    constexpr_namespace = _get_constexpr_namespace(config)

    headers = [
        '#include "app_framework.hpp"',
        '#include "libxr.hpp"',
        "",
        "// Module headers",
    ] + [f'#include "{mod}.hpp"' for mod in modules]
    if config.get("constexprs"):
        headers.append('#include "xrobot_constexpr.hpp"')

    body = [
        f"static void XRobotMain(LibXR::HardwareContainer &{hw_var}) {{",
        "  using namespace LibXR;",
        "  ApplicationManager appmgr;",
        "",
        "  // Auto-generated module instantiations",
    ]

    module_entries = config.get("modules", [])
    if not isinstance(module_entries, list):
        raise TypeError("[ERROR] 'modules' must be a list of module instances")

    auto_inst_index = {}

    for entry in module_entries:
        if not isinstance(entry, dict):
            raise TypeError("[ERROR] each item in 'modules' must be a mapping")

        mod = entry.get("name")
        if not isinstance(mod, str) or not mod.strip():
            raise ValueError("[ERROR] each module entry requires non-empty string 'name'")
        mod = mod.strip()

        inst_id = entry.get("id")
        if inst_id is not None:
            if not isinstance(inst_id, str) or not inst_id.strip():
                raise ValueError(f"[ERROR] module '{mod}' has invalid non-empty string 'id'")
            instance_name = inst_id.strip()
        else:
            idx = auto_inst_index.get(mod, 0)
            instance_name = f"{mod.lower()}{idx}" if idx > 0 else mod.lower()
            auto_inst_index[mod] = idx + 1

        if mod not in modules:
            print(f"[WARN] Module {mod} not included in the provided list.")
            continue

        args_dict = _require_mapping(
            entry.get("constructor_args", {}),
            f"constructor_args for module '{mod}'",
        )
        args_list = [
            _format_cpp_value(v, k, constexpr_namespace=constexpr_namespace)
            for k, v in args_dict.items()
        ]

        tmpl_dict = _require_mapping(entry.get("template_args", {}), f"template_args for module '{mod}'")
        tmpl_params = [
            _format_template_arg(v, constexpr_namespace=constexpr_namespace)
            for _, v in tmpl_dict.items()
        ]

        body.append(
            _format_module_instance_statement(mod, tmpl_params, instance_name, hw_var, args_list)
        )

    body += [
        "",
        "  while (true) {",
        "    appmgr.MonitorAll();",
        f"    Thread::Sleep({sleep_ms});",
        "  }",
        "}",
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
    if not isinstance(config, dict):
        raise TypeError("[ERROR] top-level config must be a mapping")

    module_entries = config.get("modules", [])
    if module_entries is None:
        return []
    if not isinstance(module_entries, list):
        raise TypeError("[ERROR] 'modules' must be a list of module instances")

    selected_modules = []
    seen_modules = set()
    for entry in module_entries:
        if not isinstance(entry, dict):
            raise TypeError("[ERROR] each item in 'modules' must be a mapping")

        mod = entry.get("name")
        if not isinstance(mod, str) or not mod.strip():
            raise ValueError("[ERROR] each module entry requires non-empty string 'name'")

        mod = mod.strip()
        if mod in seen_modules:
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

    try:
        if config_path.exists():
            print(f"[INFO] Using existing configuration file: {config_path}")
            config_data = _load_config_file(config_path)
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

        constexpr_code = _generate_constexpr_header(config_data)
        if constexpr_code is not None:
            constexpr_path = output_path.with_name("xrobot_constexpr.hpp")
            constexpr_path.write_text(constexpr_code, encoding="utf-8")
            print(f"[SUCCESS] Generated constexpr header: {constexpr_path}")
    except (TypeError, ValueError, FileNotFoundError) as e:
        parser.exit(1, f"{e}\n")

if __name__ == "__main__":
    main()
