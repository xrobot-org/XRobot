"""BSP layout: root discovery, the entry source, application configs and the
generated header's input list.

A BSP root is the nearest directory containing ``Modules/modules.yaml``. The
entry is the one source under ``User/`` that calls ``XROBOT_MAIN()``. Every
``*.yaml`` under ``User/`` except ``User/libxr_config.yaml`` is an application
configuration. The generated ``User/xrobot_main.hpp`` is not committed; its
first lines name the configuration it was generated from and every file the
generator read, so LibXR's CMake can refuse a stale header without Python.
"""

import os
import re
from pathlib import Path

from xr_syntax.cpp import identifier_occurrences

MODULES_YAML = Path("Modules/modules.yaml")
SOURCE_SUFFIXES = (".c", ".cc", ".cpp", ".cxx")
HEADER_LINE = re.compile(r'^// xrobot: (config|depends) "([^"]+)"$')


class ProjectError(ValueError):
    pass


def find_root(start="."):
    start = Path(start).resolve()
    for folder in [start] + list(start.parents):
        if (folder / MODULES_YAML).is_file():
            return folder
    raise ProjectError(
        f"No XRobot BSP found at or above {start} (no Modules/modules.yaml); run "
        "`xrobot init` in the BSP root to create one"
    )


class Project:
    def __init__(self, root):
        self.root = Path(root).resolve()
        self.modules_dir = self.root / "Modules"
        self.modules_yaml = self.modules_dir / "modules.yaml"
        self.sources_yaml = self.modules_dir / "sources.yaml"
        self.lock = self.root / "xrobot.lock"
        self.user = self.root / "User"
        self.header = self.user / "xrobot_main.hpp"
        self.default_config = self.user / "xrobot.yaml"
        self.libxr_config = self.user / "libxr_config.yaml"

    @classmethod
    def discover(cls, start="."):
        return cls(find_root(start))

    def relative(self, path):
        path = Path(path).resolve()
        try:
            return path.relative_to(self.root).as_posix()
        except ValueError:
            return path.as_posix()

    def configs(self):
        """Every application configuration of this BSP, sorted."""
        if not self.user.is_dir():
            return []
        found = [p for p in self.user.rglob("*.yaml") if p.resolve() != self.libxr_config.resolve()]
        return sorted(found, key=lambda p: self.relative(p))

    def entry(self):
        """The unique User/ source calling XROBOT_MAIN()."""
        callers = []
        if self.user.is_dir():
            for path in sorted(self.user.rglob("*")):
                if path.suffix not in SOURCE_SUFFIXES or not path.is_file():
                    continue
                text = path.read_text(encoding="utf-8-sig", errors="surrogateescape")
                if "XROBOT_MAIN" not in text:
                    continue
                if _calls_xrobot_main(text, path):
                    callers.append(path)
        if not callers:
            raise ProjectError(
                "No source under User/ calls XROBOT_MAIN(); the entry source must "
                "call it once after registering its hardware with XR_REGISTER"
            )
        if len(callers) > 1:
            raise ProjectError(
                "Several sources under User/ call XROBOT_MAIN(): {}; a BSP has "
                "exactly one entry".format(", ".join(self.relative(p) for p in callers))
            )
        return callers[0]

    def header_lines(self, config, depends):
        """The input list written after ``#pragma once``."""
        lines = [f'// xrobot: config "{_header_relative(config, self.header)}"']
        lines += [f'// xrobot: depends "{_header_relative(p, self.header)}"' for p in depends]
        return lines

    def header_state(self):
        """Freshness of User/xrobot_main.hpp, mirroring LibXR's CMake check."""
        state = {
            "path": self.relative(self.header),
            "status": "missing",
            "config": None,
            "newer": [],
            "missing": [],
        }
        if not self.header.is_file():
            return state
        config, depends = read_header_inputs(self.header)
        if config is None:
            state["status"] = "unreadable"
            return state
        base = self.header.parent
        state["config"] = self.relative(base / config)
        header_time = self.header.stat().st_mtime_ns
        for relative in [config] + depends:
            path = base / relative
            if not path.is_file():
                state["missing"].append(self.relative(path))
            elif path.stat().st_mtime_ns > header_time:
                state["newer"].append(self.relative(path))
        state["status"] = "stale" if state["newer"] or state["missing"] else "fresh"
        return state

    def header_selection(self):
        """The configuration named by the generated header, or None without a readable header."""
        if not self.header.is_file():
            return None
        config, _ = read_header_inputs(self.header)
        return (self.header.parent / config).resolve() if config is not None else None

    def selected_config(self):
        """The product the current header was generated for, else User/xrobot.yaml.

        A header naming a configuration that no longer exists is an error; the
        selection is not replaced by the default.
        """
        chosen = self.header_selection()
        if chosen is None:
            return self.default_config
        if not chosen.is_file():
            raise ProjectError(
                f"{self.relative(self.header)} was generated for {self.relative(chosen)}, which does not exist; select a configuration "
                "with `xrobot gen -c <config>`"
            )
        return chosen


def _calls_xrobot_main(text, path):
    """A call in code (preprocessor lines, comments and literals are not calls)."""
    return any(o.text == "XROBOT_MAIN" and o.following == "(" for o in identifier_occurrences(text))


def _header_relative(path, header):
    base = Path(os.path.abspath(header)).parent
    try:
        return Path(os.path.relpath(os.path.abspath(path), base)).as_posix()
    except ValueError:  # another Windows drive: no relative path exists
        return Path(os.path.abspath(path)).as_posix()


def read_header_inputs(header):
    """Return (config, [depends]) from a generated header, or (None, []) if unreadable."""
    config, depends = None, []
    try:
        lines = Path(header).read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return None, []
    for line in lines[1:]:
        if not line.startswith("// xrobot:"):
            break
        match = HEADER_LINE.match(line)
        if not match:
            return None, []
        kind, value = match.groups()
        if kind == "config":
            if config is not None:
                return None, []
            config = value
        else:
            depends.append(value)
    if config is None or not depends:
        return None, []
    return config, depends
