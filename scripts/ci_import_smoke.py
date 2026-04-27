#!/usr/bin/env python3
"""Import all top-level xrobot modules from the installed package."""

import importlib
import sys
from pathlib import Path


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    src_dir = repo_root / "src" / "xrobot"

    failures = []
    imported = []

    for path in sorted(src_dir.glob("*.py")):
        if path.name == "__init__.py":
            continue

        module_name = f"xrobot.{path.stem}"
        try:
            importlib.import_module(module_name)
            imported.append(module_name)
        except Exception as error:
            failures.append((module_name, error))

    print(f"Imported {len(imported)} modules.")
    for module_name in imported:
        print(f"OK  {module_name}")

    if failures:
        print(f"Failed to import {len(failures)} modules.", file=sys.stderr)
        for module_name, error in failures:
            print(f"ERR {module_name}: {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
