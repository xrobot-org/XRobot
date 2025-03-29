import argparse
from pathlib import Path

HEADER_TEMPLATE = """#pragma once

// clang-format off
/* === MODULE MANIFEST ===
module_name: {module_name}
module_description: {description}
{constructor_line}{hardware_line}{repo_line}=== END MANIFEST === */
// clang-format on

#include "app_framework.hpp"

template <typename HardwareContainer>
class {module_name} : public LibXR::Application {{
public:
  {constructor_signature} {{
    // Hardware initialization example:
    // auto dev = hw.template Find<LibXR::GPIO>("led");
  }}

private:
}};
"""

README_TEMPLATE = """# {module_name}

{description}

## Required Hardware
{hardware}

{repo_block}
"""

def create_module(module_name: str,
                  description: str,
                  required_hardware: list[str],
                  repository: str,
                  output_dir: Path = Path("Modules")):
    """Create a new XRobot module with standard structure"""
    mod_dir = output_dir / module_name
    mod_dir.mkdir(parents=True, exist_ok=True)

    # Template configuration for constructor arguments
    constructor_line = "constructor_args:\n  - name: \"your_arg_name_here\"\n"
    hardware_line = f"required_hardware: {' '.join(required_hardware)}\n" if required_hardware else ""
    repo_line = f"repository: {repository}\n" if repository else ""

    # Constructor signature template
    constructor_signature = f"{module_name}(HardwareContainer &hw, LibXR::ApplicationManager &app)"

    # Generate header file
    hpp_code = HEADER_TEMPLATE.format(
        module_name=module_name,
        description=description,
        constructor_line=constructor_line,
        hardware_line=hardware_line,
        repo_line=repo_line,
        constructor_signature=constructor_signature
    )

    # Generate documentation
    readme_code = README_TEMPLATE.format(
        module_name=module_name,
        description=description,
        hardware=", ".join(required_hardware) or "None",
        repo_block=f"## Repository\n{repository}" if repository else ""
    )

    (mod_dir / f"{module_name}.hpp").write_text(hpp_code, encoding="utf-8")
    (mod_dir / "README.md").write_text(readme_code, encoding="utf-8")

    # Generate build configuration
    cmake_code = f"""# CMakeLists.txt for {module_name}

# Add module to include path
target_include_directories(xr PUBLIC ${{CMAKE_CURRENT_LIST_DIR}})

# Auto-include source files
file(GLOB MODULE_{module_name.upper()}_SRC
    "${{CMAKE_CURRENT_LIST_DIR}}/*.cpp"
    "${{CMAKE_CURRENT_LIST_DIR}}/*.c"
)

target_sources(xr PRIVATE ${{MODULE_{module_name.upper()}_SRC}})
"""
    (mod_dir / "CMakeLists.txt").write_text(cmake_code, encoding="utf-8")

    print(f"[OK] Module {module_name} generated at {mod_dir}")


def main():
    parser = argparse.ArgumentParser(description="Create a new XRobot module")
    parser.add_argument("module_name", help="Module name (must match directory name)")
    parser.add_argument("--desc", default="No description provided", help="Module description")
    parser.add_argument("--hw", nargs="*", default=[], help="Required hardware interfaces (logical names)")
    parser.add_argument("--repo", default="", help="GitHub repository URL (optional)")
    parser.add_argument("--out", default="Modules", help="Output directory (default: Modules/)")

    args = parser.parse_args()
    create_module(args.module_name, args.desc, args.hw, args.repo, Path(args.out))


if __name__ == "__main__":
    main()