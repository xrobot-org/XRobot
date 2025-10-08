#!/usr/bin/env python3
"""
xrobot_setup.py - XRobot one-click automation script for initializing modules.yaml, sources.yaml, auto-fetching modules, and generating main C++ code.

This script:
1. Ensures `Modules/modules.yaml` and `Modules/sources.yaml` exist, generating templates if missing.
2. Reads module list from modules.yaml.
3. Invokes the CLI tools to fetch all modules and dependencies.
4. Invokes code generation for the main C++ file.

Intended for rapid setup of an XRobot project workspace.
"""

import argparse
import sys
import subprocess
from pathlib import Path
from typing import List
from xrobot.SourceManager import SourceManager, load_yaml  # Your library

MODULES_DIR = Path("Modules")
MODULES_CONFIG = MODULES_DIR / "modules.yaml"
SOURCES_CONFIG = MODULES_DIR / "sources.yaml"
OUTPUT_CPP = Path("User/xrobot_main.hpp")
INIT_MODULE_CLI = "xrobot_init_mod"
GENERATE_MAIN_CLI = "xrobot_gen_main"

MODULES_YAML_TEMPLATE = """# XRobot modules.yaml template. Each line: full module name (namespace/ModuleName[@ref]):
# Example:
#   - xrobot-org/BlinkLED
#   - your-namespace/YourModule@dev
modules:
  - xrobot-org/BlinkLED
"""

SOURCES_YAML_TEMPLATE = """# XRobot sources.yaml template. List your module repository index.yaml files, supports official and custom mirrors.
# Official example (already filled in by default):
sources:
  - url: https://xrobot-org.github.io/xrobot-modules/index.yaml
    priority: 0
"""

def ensure_modules_and_sources():
    """
    Ensure Modules/modules.yaml and Modules/sources.yaml exist, creating templates if missing.
    Exits after creation, instructing user to edit before proceeding.
    """
    created = False
    if not MODULES_CONFIG.exists():
        MODULES_CONFIG.parent.mkdir(parents=True, exist_ok=True)
        MODULES_CONFIG.write_text(MODULES_YAML_TEMPLATE, encoding="utf-8")
        print(f"[INFO] Created default {MODULES_CONFIG}")
        print("Please edit this file; each line should be a full module name like:")
        print("  - xrobot-org/BlinkLED")
        print("  - your-namespace/YourModule@dev")
        created = True

    if not SOURCES_CONFIG.exists():
        SOURCES_CONFIG.parent.mkdir(parents=True, exist_ok=True)
        SOURCES_CONFIG.write_text(SOURCES_YAML_TEMPLATE, encoding="utf-8")
        print(f"[INFO] Created default {SOURCES_CONFIG}")
        print("Please configure sources index.yaml for official or custom/private mirrors.\nDefault official source already included.")
        created = True

    if created:
        sys.exit(0)  # Exit after template creation, waiting for user edit

def run_subprocess(cmd: list[str]):
    """
    Execute an external command, log and catch errors.
    """
    print(f"[EXEC] {' '.join(cmd)}")
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Command failed: {' '.join(cmd)}\n{e}")
        sys.exit(e.returncode if hasattr(e, "returncode") else 1)

def extract_modules() -> List[str]:
    """
    Extract the list of module full names from modules.yaml.
    """
    if not MODULES_CONFIG.exists():
        return []
    data = load_yaml(MODULES_CONFIG)
    return [m for m in data.get("modules", []) if isinstance(m, str) and "/" in m]

def main():
    print("Starting XRobot auto-configuration...")

    parser = argparse.ArgumentParser(description="XRobot workspace one-click automation")
    parser.add_argument("--config", help="Path or URL to constructor config YAML")
    args = parser.parse_args()

    # Step 0: Ensure the two core config files exist
    ensure_modules_and_sources()

    # Step 1: Validate and parse the module list
    modules = extract_modules()
    if not modules:
        print(f"[ERROR] No valid modules found in {MODULES_CONFIG}, please edit and retry.")
        sys.exit(1)

    # Step 2: Fetch repositories and all dependencies recursively
    run_subprocess([
        INIT_MODULE_CLI,
        "--config", str(MODULES_CONFIG),
        "--directory", str(MODULES_DIR),
        "--sources", str(SOURCES_CONFIG)
    ])

    # Step 3: Generate main function code
    run_subprocess([
        GENERATE_MAIN_CLI,
        "--output", str(OUTPUT_CPP)
    ])

    print(f"\nAll done! Main function generated at: {OUTPUT_CPP}")

if __name__ == "__main__":
    main()
