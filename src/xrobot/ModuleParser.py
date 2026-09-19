#!/usr/bin/env python3
"""
ModuleManifest: Utility to parse and access XRobot module manifest from .hpp header or directory.

Features:
- Class-based manifest object with property access.
- Utilities for parsing from header file or folder.
- Supports V1/V2 manifest in C++ comments.
- Can be used as both a library and a CLI tool.

Usage as CLI:
    python module_manifest.py --path Modules/BlinkLED
    python module_manifest.py --path Modules/BlinkLED/BlinkLED.hpp
"""

import yaml
from pathlib import Path
from collections import OrderedDict
from typing import Optional, Dict, Union, Any

from xr_syntax.cpp import CppDocument


_MANIFEST_START_MARKERS = (
    "=== MODULE MANIFEST V2 ===",
    "=== MODULE MANIFEST ===",
)
_MANIFEST_END_MARKER = "=== END MANIFEST ==="


class ModuleManifest:
    """
    XRobot single module manifest parsed object.
    Supports attribute access, dictionary export, and pretty print.
    """
    def __init__(self, manifest: dict, path: Optional[Path] = None):
        self.manifest = manifest or {}
        self.path = path

    @property
    def description(self) -> str:
        return self.manifest.get("module_description", "")

    @property
    def constructor_args(self) -> Union[list, dict, str]:
        return self.manifest.get("constructor_args", [])

    @property
    def template_args(self) -> Union[list, dict, str]:
        return self.manifest.get("template_args", [])

    @property
    def required_hardware(self) -> list:
        hw = self.manifest.get("required_hardware", [])
        if isinstance(hw, str):
            return [hw]
        elif hw is None:
            return []
        else:
            return hw

    @property
    def depends(self) -> list:
        dep = self.manifest.get("depends", [])
        if isinstance(dep, str):
            return [dep]
        elif dep is None:
            return []
        else:
            return dep

    def as_dict(self) -> dict:
        return dict(self.manifest)

    def __repr__(self):
        return f"<ModuleManifest path={self.path} desc={self.description[:20]}>"


def _manifest_block_from_comment(comment: str) -> Optional[str]:
    """
    Extract the YAML payload from one C++ comment containing a V1/V2 manifest.
    """
    folded = comment.casefold()
    candidates = []
    for marker in _MANIFEST_START_MARKERS:
        index = folded.find(marker.casefold())
        if index >= 0:
            candidates.append((index, marker))
    if not candidates:
        return None

    start, marker = min(candidates, key=lambda item: item[0])
    body_start = start + len(marker)
    body_end = folded.find(_MANIFEST_END_MARKER.casefold(), body_start)
    if body_end < 0:
        return None
    return comment[body_start:body_end].strip()


def parse_manifest_from_header(header_path: Path) -> Optional[ModuleManifest]:
    """
    Parse a manifest block from C++ comments in a .hpp file.
    Supports V1/V2 manifest format.
    """
    try:
        source = header_path.read_bytes()
    except Exception as e:
        print(f"[ERROR] Failed to read {header_path}: {e}")
        return None

    document = CppDocument.parse(source, source_name=str(header_path))
    manifest_block = None
    for comment in document.comments():
        manifest_block = _manifest_block_from_comment(comment.text)
        if manifest_block is not None:
            break
    if manifest_block is None:
        return None

    try:
        data = yaml.safe_load(manifest_block)
        if not isinstance(data, dict):
            return None
        return ModuleManifest(data, path=header_path)
    except yaml.YAMLError as e:
        print(f"[ERROR] YAML parse error in {header_path}:\n{e}")
        return None


def parse_module_folder(folder: Path) -> Optional[ModuleManifest]:
    """
    Parse a module folder (automatically finds the .hpp file), returns ModuleManifest.
    """
    if not folder.is_dir():
        return None
    hpp_name = folder.name + ".hpp"
    hpp_path = folder / hpp_name
    if not hpp_path.exists():
        return None
    return parse_manifest_from_header(hpp_path)


def parse_constructor_args(args: Any) -> OrderedDict:
    """
    Convert constructor_args or template_args field to OrderedDict.
    Supports dict, list, or str YAML formats.
    """
    args_ordered = OrderedDict()
    if isinstance(args, dict):
        for k, v in args.items():
            args_ordered[k] = v
    elif isinstance(args, list):
        for item in args:
            if isinstance(item, dict):
                for k, v in item.items():
                    args_ordered[k] = v
            elif isinstance(item, str):
                args_ordered[item] = ""
    elif isinstance(args, str):
        args_ordered[args] = ""
    return args_ordered


def load_single_module(path: Path) -> Optional[ModuleManifest]:
    """
    Parse a single module (can be a directory or a .hpp file).
    """
    if path.is_dir():
        return parse_module_folder(path)
    elif path.is_file() and path.suffix == ".hpp":
        return parse_manifest_from_header(path)
    else:
        return None


def print_manifest(manifest: ModuleManifest, name: Optional[str] = None):
    """
    Pretty-print manifest info for a single module (terminal-friendly).
    """
    title = name or (manifest.path.name if manifest.path else "(unknown)")
    print(f"\n=== Module: {title} ===")
    print(f"Description       : {manifest.description or '(no description)'}")

    # Constructor arguments
    args = manifest.constructor_args
    print("\nConstructor Args  : ", end="")
    if isinstance(args, list) and all(isinstance(a, dict) for a in args):
        for item in args:
            fields = ", ".join(f"{k}={v}" for k, v in item.items())
            print(f"\n  - {fields}")
    elif isinstance(args, dict):
        for k, v in args.items():
            print(f"\n  - {k}={v}")
    elif isinstance(args, str):
        print(args)
    else:
        print("(invalid format or not specified)")

    # Template arguments
    template_args = manifest.template_args
    if template_args:
        print("\nTemplate Args     : ", end="")
        if isinstance(template_args, list) and all(isinstance(a, dict) for a in template_args):
            for item in template_args:
                fields = ", ".join(f"{k}={v}" for k, v in item.items())
                print(f"\n  - {fields}")
        elif isinstance(template_args, dict):
            for k, v in template_args.items():
                print(f"\n  - {k}={v}")
        elif isinstance(template_args, str):
            print(template_args)
        else:
            print("(invalid format)")
    hardware = manifest.required_hardware
    print(f"\nRequired Hardware : {', '.join(hardware) if hardware else 'None'}")
    depends = manifest.depends
    if depends:
        print(f"Depends           : {', '.join(depends)}")
    else:
        print("Depends           : None")


def main():
    import argparse
    parser = argparse.ArgumentParser(description="XRobot module manifest inspection tool")
    parser.add_argument("--path", "-p", required=True, help="Module directory or .hpp path")
    args = parser.parse_args()
    manifest = load_single_module(Path(args.path))
    if manifest:
        print_manifest(manifest)
    else:
        print("[ERROR] Module manifest not found or invalid.")


if __name__ == "__main__":
    main()
