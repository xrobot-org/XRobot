#!/usr/bin/env python3
"""
xrobot_module_generator.py - Quickly create a new XRobot module with standardized manifest (V2).

Features:
- Generates C++ header with MANIFEST block, README, and CMakeLists.txt for the module.
- Supports command-line arguments for class name, description, hardware, constructor args, template args, and dependencies.

Usage:
    python xrobot_module_generator.py MyModule --desc "A blinking LED" --hw led --constructor id=foo gpio=LED
"""

import argparse
from pathlib import Path
from typing import List, Dict, Any, Optional

HEADER_TEMPLATE = """#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: {description}
{constructor_args_block}
{template_args_block}
{hardware_block}
{depends_block}
=== END MANIFEST === */
// clang-format on

#include "app_framework.hpp"

class {class_name} : public LibXR::Application {{
public:
  {class_name}(LibXR::HardwareContainer &hw, LibXR::ApplicationManager &app) {{
    // Hardware initialization example:
    // auto dev = hw.template Find<LibXR::GPIO>("led");
  }}

  void OnMonitor() override {{}}

private:
}};
"""

README_TEMPLATE = """# {class_name}

{description}

## Required Hardware
{hardware}

## Constructor Arguments
{constructor_args}

## Template Arguments
{template_args}

## Depends
{depends}
"""

GITHUB_ACTIONS_WORKFLOW = """name: XRobot Module Build Test

on:
  push:
  pull_request:
  schedule:
    - cron: '0 3 1 * *'  # 每月1日凌晨3点（UTC）自动触发

jobs:
  build:
    runs-on: ubuntu-latest
    container:
      image: ghcr.io/xrobot-org/docker-image-linux:main
      options: --user 0

    env:
      XR_MODULE_NAME: ${{ github.event.repository.name }}

    steps:
      - name: Checkout current module repo to ./Modules/${{ env.XR_MODULE_NAME }}
        uses: actions/checkout@v4
        with:
          path: Modules/${{ env.XR_MODULE_NAME }}

      - name: Create main.cpp
        run: |
          cat > main.cpp <<'EOF'
          #include <iostream>
          #include "xrobot_main.hpp"
          #include "libxr.hpp"

          int main() {
              LibXR::PlatformInit();
              LibXR::STDIO::Printf("This is XRobot Module Template Test\\n");
              LibXR::HardwareContainer hw;
              XRobotMain(hw);
              return 0;
          }
          EOF

      - name: Create minimal CMakeLists.txt
        run: |
          cat > CMakeLists.txt <<'EOF'
          project(xrobot_mod_test CXX)
          set(CMAKE_CXX_STANDARD 17)
          add_executable(xr_test main.cpp)
          set(XROBOT_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Modules/)
          add_subdirectory(libxr)
          target_include_directories(xr_test PUBLIC $<TARGET_PROPERTY:xr,INTERFACE_INCLUDE_DIRECTORIES> ${CMAKE_SOURCE_DIR}/User)
          target_link_libraries(xr_test PUBLIC xr)
          EOF

      - name: Pull libxr to ./libxr
        run: git clone --depth=1 https://github.com/Jiu-xiao/libxr ./libxr

      - name: Setup Python & Install deps
        run: |
          python3 -m pip install --upgrade pip
          pip3 install pyyaml requests

      - name: Add XRobot tools to PATH
        run: |
          echo "$HOME/.local/bin" >> $GITHUB_PATH

      - name: Install xrobot toolchain
        run: |
          pip3 install xrobot libxr

      - name: Run xrobot_setup
        run: |
          xrobot_setup || true

      - name: Run xrobot_init_mod
        run: |
          xrobot_init_mod

      - name: Add BlinkLED module
        run: |
          xrobot_add_mod BlinkLED --instance-id BlinkLED_0

      - name: Add this repo module
        run: |
          xrobot_add_mod ${{ env.XR_MODULE_NAME }} && cat User/xrobot.yaml

      - name: Generate main again
        run: |
          xrobot_setup

      - name: Build
        run: |
          mkdir -p build
          cd build
          cmake ..
          make

      - name: Create Tag via GitHub API
        if: ${{ success() && github.event_name != 'pull_request' }}
        uses: actions/github-script@v7
        with:
          github-token: ${{ secrets.GITHUB_TOKEN }}
          script: |
            const tagPrefix = 'auto-'
            const date = new Date();
            const yyyy = date.getUTCFullYear();
            const mm = String(date.getUTCMonth() + 1).padStart(2, '0');
            const dd = String(date.getUTCDate()).padStart(2, '0');
            const HH = String(date.getUTCHours()).padStart(2, '0');
            const MM = String(date.getUTCMinutes()).padStart(2, '0');
            const SS = String(date.getUTCSeconds()).padStart(2, '0');
            const tagName = `${tagPrefix}${yyyy}${mm}${dd}-${HH}${MM}${SS}`;

            // 获取当前 commit sha
            const sha = process.env.GITHUB_SHA;

            // 创建 tag reference
            await github.rest.git.createRef({
              owner: context.repo.owner,
              repo: context.repo.repo,
              ref: `refs/tags/${tagName}`,
              sha: sha
            });
            console.log(`Created tag: ${tagName} on sha: ${sha}`);
"""

def yaml_block(obj: Any, key: str, indent: int = 2) -> str:
    """
    Output a compliant YAML field block:
    - Empty list/dict/None: key: []
    - Non-empty list/dict: block format, indented
    - String: key: value
    """
    import yaml
    if obj is None or obj == [] or obj == {}:
        return f"{key}: []"
    if isinstance(obj, str):
        return f"{key}: {obj}"
    text = yaml.dump(obj, allow_unicode=True, sort_keys=False, default_flow_style=False)
    pad = " " * indent
    lines = text.splitlines()
    return f"{key}:\n" + "\n".join(pad + l if l.strip() else l for l in lines)

def _auto_type(val: str):
    """Try to convert to int/float/bool if possible, else return original string."""
    v = val.strip()
    if v.lower() in ("true", "false"):
        return v.lower() == "true"
    try:
        if v.startswith("0x") or v.startswith("0X"):
            return int(v, 16)
        if '.' in v:
            return float(v)
        return int(v)
    except Exception:
        return v  # fallback as string

def parse_arg(arglist):
    """
    Support name=value (becomes {name: value, type detected}), or name (becomes {name: ""})
    """
    result = []
    for s in arglist:
        if '=' in s:
            k, v = s.split('=', 1)
            result.append({k.strip(): _auto_type(v)})
        else:
            result.append({s.strip(): ""})
    return result

def format_args(args: Optional[List[Dict[str, Any]]]) -> str:
    """Format constructor or template args as markdown list"""
    if not args:
        return "None"
    lines = []
    for arg in args:
        key = list(arg.keys())[0]
        val = arg[key]
        if val != "":
            lines.append(f"- `{key}`: {val}")
        else:
            lines.append(f"- `{key}`")
    return "\n".join(lines)

def format_depends(depends: Optional[List[str]]) -> str:
    if not depends:
        return "None"
    return "\n".join(f"- {dep}" for dep in depends)

def create_module(
    class_name: str,
    description: str,
    required_hardware: List[str],
    constructor_args: Optional[List[Dict[str, Any]]] = None,
    template_args: Optional[List[Dict[str, Any]]] = None,
    depends: Optional[List[str]] = None,
    output_dir: Path = Path("Modules")
):
    """Create a new XRobot module with standardized manifest and build files."""
    mod_dir = output_dir / class_name
    mod_dir.mkdir(parents=True, exist_ok=True)

    constructor_args = constructor_args or []
    template_args = template_args or []
    depends = depends or []
    required_hardware = required_hardware or []

    hpp_code = HEADER_TEMPLATE.format(
        class_name=class_name,
        description=description,
        constructor_args_block=yaml_block(constructor_args, "constructor_args"),
        template_args_block=yaml_block(template_args, "template_args"),
        hardware_block=yaml_block(required_hardware, "required_hardware"),
        depends_block=yaml_block(depends, "depends"),
    )

    readme_code = README_TEMPLATE.format(
        class_name=class_name,
        description=description,
        hardware=", ".join(required_hardware) if required_hardware else "None",
        constructor_args=format_args(constructor_args),
        template_args=format_args(template_args),
        depends=format_depends(depends),
    )

    (mod_dir / f"{class_name}.hpp").write_text(hpp_code, encoding="utf-8")
    (mod_dir / "README.md").write_text(readme_code, encoding="utf-8")

    # Generate build configuration
    cmake_code = f"""# CMakeLists.txt for {class_name}

# Add module to include path
target_include_directories(xr PUBLIC ${{CMAKE_CURRENT_LIST_DIR}})

# Auto-include source files
file(GLOB MODULE_{class_name.upper()}_SRC CONFIGURE_DEPENDS
  "${{CMAKE_CURRENT_LIST_DIR}}/*.cpp"
  "${{CMAKE_CURRENT_LIST_DIR}}/*.cxx"
  "${{CMAKE_CURRENT_LIST_DIR}}/*.cc"
  "${{CMAKE_CURRENT_LIST_DIR}}/*.c"
)

target_sources(xr PRIVATE ${{MODULE_{class_name.upper()}_SRC}})

# INTERFACE deps shell
get_filename_component(_MODULE_DIRNAME ${{CMAKE_CURRENT_LIST_DIR}} NAME)
set(_DEPS_TARGET "${{_MODULE_DIRNAME}}_deps")
add_library(${{_DEPS_TARGET}} INTERFACE)

# find_package(YourPkg REQUIRED COMPONENTS a b c)

# target_link_libraries(${{_DEPS_TARGET}} INTERFACE
#   # YourLibA
#   # YourLibB
# )

# target_compile_definitions(${{_DEPS_TARGET}} INTERFACE
#   # FOO=1
# )
# target_compile_options(${{_DEPS_TARGET}} INTERFACE
#   # -Wall
# )

# Register to global
set_property(GLOBAL APPEND PROPERTY XR_MODULE_DEPS ${{_DEPS_TARGET}})
"""
    (mod_dir / "CMakeLists.txt").write_text(cmake_code, encoding="utf-8")

    # Generate GitHub Actions workflow
    workflow_dir = mod_dir / ".github" / "workflows"
    workflow_dir.mkdir(parents=True, exist_ok=True)
    (workflow_dir / "build.yml").write_text(GITHUB_ACTIONS_WORKFLOW, encoding="utf-8")

    print(f"[OK] Module {class_name} generated at {mod_dir}")

def main():
    parser = argparse.ArgumentParser(
        description="Create a new XRobot module (standardized manifest structure)"
    )
    parser.add_argument("class_name", help="Module class/dir name (should match directory name)")
    parser.add_argument("--desc", default="No description provided", help="Module description")
    parser.add_argument("--hw", nargs="*", default=[], help="Required hardware interfaces (logical names)")
    parser.add_argument("--constructor", nargs="*", default=[], help="Constructor args, e.g., id=foo gpio=LED")
    parser.add_argument("--template", nargs="*", default=[], help="Template args, e.g., T=int U=double")
    parser.add_argument("--depends", nargs="*", default=[], help="Dependencies (other modules)")
    parser.add_argument("--out", default="Modules", help="Output directory (default: Modules/)")

    args = parser.parse_args()

    constructor_args = parse_arg(args.constructor)
    template_args = parse_arg(args.template)

    create_module(
        class_name=args.class_name,
        description=args.desc,
        required_hardware=args.hw,
        constructor_args=constructor_args,
        template_args=template_args,
        depends=args.depends,
        output_dir=Path(args.out)
    )

if __name__ == "__main__":
    main()
