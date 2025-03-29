import argparse
import subprocess
import yaml
from pathlib import Path

CONFIG_TEMPLATE = """# XRobot module configuration example
modules:
  - name: BlinkLED
    repo: https://github.com/xrobot-org/BlinkLED
"""


def execute_git_command(cmd: list[str], workdir: Path = None):
    """Execute git commands with error handling."""
    try:
        subprocess.run(cmd, cwd=workdir, check=True)
    except subprocess.CalledProcessError as e:
        print(f"[ERROR] Git command failed: {' '.join(cmd)}\n{str(e)}")


def sync_module(name: str, repo: str, base_dir: Path):
    """Clone or update a module repository."""
    module_path = base_dir / name

    # Update existing repository
    if module_path.exists() and (module_path / ".git").exists():
        print(f"[INFO] Updating module: {name}")
        execute_git_command(["git", "pull"], workdir=module_path)
    # Clone new repository
    else:
        print(f"[INFO] Cloning new module: {name}")
        execute_git_command(["git", "clone", repo, str(module_path)])


def load_configuration(config_path: Path) -> list[dict]:
    """Load module configuration from YAML file."""
    if not config_path.exists():
        print(f"[WARN] Configuration file not found, creating template: {config_path}")
        config_path.write_text(CONFIG_TEMPLATE, encoding="utf-8")
        print("[INFO] Please edit the configuration file and rerun this script.")
        return []

    with config_path.open(encoding="utf-8") as f:
        config_data = yaml.safe_load(f)
    return config_data.get("modules", [])


def main():
    """Main entry point for module synchronization."""
    parser = argparse.ArgumentParser(
        description="XRobot module synchronization tool",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument("--config", "-c", default="modules.yaml",
                        help="Path to module configuration file")
    parser.add_argument("--directory", "-d", default="Modules",
                        help="Output directory for module repositories")

    args = parser.parse_args()
    config_file = Path(args.config)
    module_dir = Path(args.directory)
    module_dir.mkdir(parents=True, exist_ok=True)

    modules = load_configuration(config_file)
    if not modules:
        return

    for module in modules:
        if not all(key in module for key in ("name", "repo")):
            print(f"[WARN] Skipping invalid module entry: {module}")
            continue
        sync_module(module["name"], module["repo"], module_dir)

    print("[SUCCESS] All modules processed")


if __name__ == "__main__":
    main()