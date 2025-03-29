import subprocess
import sys
from pathlib import Path
import yaml

# Configuration paths
MODULE_CONFIG = Path("./Modules/modules.yaml")
MODULE_DIR = Path("./Modules/")
OUTPUT_CPP = Path("./User/xrobot_main.hpp")
CONSTRUCTOR_CONFIG = Path("./User/xrobot.yaml")

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
        MODULE_CONFIG.write_text("""modules:
  - name: BlinkLED
    repo: https://github.com/xrobot-org/BlinkLED
""", encoding="utf-8")
        print(f"[INFO] Default config created: {MODULE_CONFIG}")
        print("Please edit the file and rerun.")
        sys.exit(0)


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