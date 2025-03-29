import re
import yaml
import argparse
from pathlib import Path
from typing import Optional, Dict, Union, OrderedDict


def parse_constructor_args(text: str) -> OrderedDict:
    """Parse constructor arguments with nested structure support.

    Returns:
        OrderedDict: Preserves argument order while maintaining nested structure awareness
    """
    args_dict = OrderedDict()
    depth = 0
    buffer = ""
    key = None
    i = 0

    while i < len(text):
        c = text[i]
        if c in "{[(":
            depth += 1
            buffer += c
        elif c in "}])":
            depth -= 1
            buffer += c
        elif c == "," and depth == 0:
            part = buffer.strip()
            if "=" in part:
                k, v = map(str.strip, part.split("=", 1))
                args_dict[k] = v
            elif key is not None:
                args_dict[key] = part
            buffer = ""
            key = None
        else:
            buffer += c
        i += 1

    # Process final item
    if buffer.strip():
        part = buffer.strip()
        if "=" in part:
            k, v = map(str.strip, part.split("=", 1))
            args_dict[k] = v
        elif key is not None:
            args_dict[key] = part

    return args_dict


def parse_manifest_from_header(header_path: Path) -> Optional[dict]:
    """Extract and parse manifest block from module header file."""
    try:
        content = header_path.read_text(encoding="utf-8-sig")  # Handle BOM
    except UnicodeDecodeError:
        content = header_path.read_text(encoding="utf-8")

    # Robust regex pattern allowing whitespace and line breaks
    pattern = re.compile(
        r"/\*\s*=== MODULE MANIFEST ===\s*[\r\n]+(.*?)[\r\n]+\s*=== END MANIFEST ===\s*\*/",
        re.DOTALL
    )
    match = pattern.search(content)
    if not match:
        return None

    manifest_block = match.group(1)
    manifest_block = "\n".join(
        [line.rstrip() for line in manifest_block.splitlines() if line.strip()]
    )

    try:
        return yaml.safe_load(manifest_block)
    except yaml.YAMLError:
        return None


def parse_module_folder(folder: Path) -> Optional[Dict[str, Union[str, list[str], dict]]]:
    """Locate and parse manifest from matching header file in module directory."""
    if not folder.is_dir():
        return None

    hpp_name = folder.name + ".hpp"
    hpp_path = folder / hpp_name
    if not hpp_path.exists():
        return None

    return parse_manifest_from_header(hpp_path)


def parse_path(path: Path) -> Dict[str, Dict]:
    """Main entry point: handles both module collections and individual modules."""
    manifests = {}

    if not path.exists():
        return manifests

    if path.is_dir():
        # Check if path is a module directory
        maybe_mod = path / (path.name + ".hpp")
        if maybe_mod.exists():
            if manifest := parse_module_folder(path):
                manifests[manifest["module_name"]] = manifest
        else:
            # Process directory of modules
            for sub in path.iterdir():
                if sub.is_dir() and (manifest := parse_module_folder(sub)):
                    manifests[manifest["module_name"]] = manifest
    return manifests


def print_manifest(name: str, manifest: dict):
    """Display formatted module information."""
    print(f"\n=== Module: {name} ===")

    print(f"Description       : {manifest.get('module_description', '(no description)')}")
    if repo := manifest.get("repository"):
        print(f"Repository        : {repo}")

    # Handle constructor arguments
    args = manifest.get("constructor_args")
    print("\nConstructor Args  : ", end="")
    if isinstance(args, list) and all(isinstance(a, dict) for a in args):
        for item in args:
            for k, v in item.items():
                print(f"\n  - {k:<18} = {v if v is not None else '(missing)'}")
    else:
        print("(invalid format)")

    # Handle hardware requirements
    hardware = manifest.get("required_hardware", [])
    if isinstance(hardware, str):
        hardware = [hardware]
    print(f"\nRequired Hardware : {', '.join(hardware) if hardware else 'None'}")


def main():
    parser = argparse.ArgumentParser(description="XRobot module manifest parser")
    parser.add_argument("--path", "-p", required=True,
                        help="Path to module directory or module collection")
    args = parser.parse_args()

    manifests = parse_path(Path(args.path))
    if not manifests:
        print("[INFO] No module manifests found")
        return

    for name, manifest in manifests.items():
        print_manifest(name, manifest)


if __name__ == "__main__":
    main()