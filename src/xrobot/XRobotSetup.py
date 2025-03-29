import subprocess
import sys
from pathlib import Path
import yaml
import argparse
import urllib.request

# Configuration paths
MODULE_CONFIG = Path("./Modules/modules.yaml")
MODULE_DIR = Path("./Modules/")
OUTPUT_CPP = Path("./User/xrobot_main.hpp")
CONSTRUCTOR_CONFIG: Path  # Type hint, no initial value

# CLI tool names
INIT_MODULE_CLI = "xrobot_init_mod"
GENERATE_MAIN_CLI = "xrobot_gen_main"


def run_subprocess(cmd: list[str]):
    """Execute external command with logging"""
    print(f"[EXEC] {' '.join(cmd)}")
    subprocess.run(cmd, check=True)


def ensure_module_config():
    """Create default module configuration if missing"""
    if not MODULE_CONFIG.exists():
        MODULE_CONFIG.parent.mkdir(parents=True, exist_ok=True)
        MODULE_CONFIG.write_text("""modules:
  - name: BlinkLED
    repo: https://github.com/xrobot-org/BlinkLED
""", encoding="utf-8")
        print(f"[INFO] Default config created: {MODULE_CONFIG}")
        print("Please edit the file and rerun.")
        sys.exit(0)

def ensure_modules_cmakelists():
    """Ensure a default CMakeLists.txt exists under Modules/, used to auto-include submodules"""
    cmake_file = MODULE_DIR / "CMakeLists.txt"
    if cmake_file.exists():
        return  # Do not overwrite existing file

    cmake_code = r'''# Automatically include all module CMakeLists.txt files under Modules/

message(STATUS "[XRobot] Scanning module directory: Modules/")

file(GLOB MODULE_DIRS RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/*)

foreach(MOD ${MODULE_DIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/${MOD}")
        if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${MOD}/CMakeLists.txt")
            message(STATUS "[XRobot] Including module: ${MOD}")
            include("${CMAKE_CURRENT_LIST_DIR}/${MOD}/CMakeLists.txt")
        endif()
    endif()
endforeach()
'''
    cmake_file.write_text(cmake_code, encoding="utf-8")
    print(f"[INFO] Generated default Modules/CMakeLists.txt at: {cmake_file}")


def resolve_config_path(user_input: str | None) -> Path:
    """Resolve config path from local path or URL"""
    default_path = Path("./User/xrobot.yaml")

    if not user_input:
        return default_path

    if user_input.startswith("http://") or user_input.startswith("https://"):
        # Download remote YAML to local path
        print(f"[INFO] Downloading config from URL: {user_input}")
        default_path.parent.mkdir(parents=True, exist_ok=True)
        urllib.request.urlretrieve(user_input, default_path)
        print(f"[INFO] Config downloaded to: {default_path}")
        return default_path

    return Path(user_input)


def extract_modules() -> list[str]:
    """Retrieve module list from configuration"""
    if not MODULE_CONFIG.exists():
        return []

    with MODULE_CONFIG.open(encoding="utf-8") as f:
        data = yaml.safe_load(f)

    return [m["name"] for m in data.get("modules", []) if "name" in m]


def main():
    """Main automation workflow"""
    print("Starting XRobot auto-configuration")

    parser = argparse.ArgumentParser(description="XRobot setup automation")
    parser.add_argument("--config", help="Path or URL to constructor config YAML")
    args = parser.parse_args()

    # Step 0: Resolve config path
    global CONSTRUCTOR_CONFIG
    CONSTRUCTOR_CONFIG = resolve_config_path(args.config)

    # Configuration validation
    ensure_module_config()
    modules = extract_modules()

    if not modules:
        print("[ERROR] No modules found in configuration")
        sys.exit(1)

    # Step 1: Repository management
    run_subprocess([
        INIT_MODULE_CLI,
        "--config", str(MODULE_CONFIG),
        "--dir", str(MODULE_DIR)
    ])

    ensure_modules_cmakelists()

    # Step 2: Generate constructor config if missing
    if not CONSTRUCTOR_CONFIG.exists():
        run_subprocess([
            GENERATE_MAIN_CLI,
            "--output", str(OUTPUT_CPP),
            "--config", str(CONSTRUCTOR_CONFIG)
        ])
        print(f"[INFO] Constructor config generated: {CONSTRUCTOR_CONFIG}")
        print("Modify the configuration and rerun to apply changes")
        sys.exit(0)

    # Step 3: Code generation
    run_subprocess([
        GENERATE_MAIN_CLI,
        "--output", str(OUTPUT_CPP),
        "--config", str(CONSTRUCTOR_CONFIG)
    ])

    print(f"\nAll done! Main function generated at: {OUTPUT_CPP}")


if __name__ == "__main__":
    main()