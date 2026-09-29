"""Create a plain C++ Module with thin package metadata, native CMake and CI."""

import re
from pathlib import Path

import yaml

CI_WORKFLOW = """name: Module CI

on:
  push:
  pull_request:
  workflow_dispatch:

jobs:
  build:
    uses: xrobot-org/XRobot/.github/workflows/module-ci.yml@v1
    with:
      template-args: '%s'
"""

CMAKE = """target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
file(GLOB MODULE_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/*.cc"
     "${CMAKE_CURRENT_LIST_DIR}/*.cxx"
     "${CMAKE_CURRENT_LIST_DIR}/*.c")
target_sources(xr PRIVATE ${MODULE_SOURCES})
"""

README = """# %(name)s

%(description)s

## Interface

The public constructor in `%(name)s.hpp` is the interface: dependencies (hardware
or other Modules, as references or pointers without defaults) come first, value
configuration with explicit defaults after. `xrobot module show .` prints it.

## Use in a BSP

```sh
xrobot module add <owner>/%(name)s
xrobot setup
xrobot instance add <owner>/%(name)s
```

`xrobot instance add` writes an instance with every parameter and its source
default; fill the dependencies (`null`) with registered hardware names or
earlier instance ids, then run `xrobot gen`.

## CI

`.github/workflows/build.yml` calls the shared XRobot Module CI, which compiles
the Module sources and one generated constructor call. The call uses `void*`
placeholders for dependencies and is never linked or executed.
"""


def _write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(text.encode("utf-8"))


def create_module(
    class_name,
    description="",
    constructor_args=None,
    template_args=None,
    depends=None,
    output_dir=Path("."),
    includes=None,
):
    if not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", class_name):
        raise ValueError("Module name must be a C++ identifier")
    folder = Path(output_dir) / class_name
    if folder.exists():
        raise ValueError(f"Refusing to overwrite existing module: {folder}")
    constructors = list(constructor_args or [])
    templates = list(template_args or [])
    for item in constructors + templates:
        if not isinstance(item, str) or not item.strip():
            raise ValueError("Constructor/template declarations must be C++ text")
    from xrobot.init_module import request

    entries = []
    for dependency in depends or []:
        parsed = request(dependency, canonical=True)
        entries.append({"id": parsed["id"], "ref": parsed["ref"] or "same-or-dev"})
    manifest = yaml.safe_dump(
        {"module_description": description, "depends": entries}, sort_keys=False, allow_unicode=True
    ).rstrip()
    lines = [
        "#pragma once",
        "",
        "// clang-format off",
        "/* === MODULE MANIFEST V2 ===",
        manifest,
        "=== END MANIFEST === */",
        "// clang-format on",
        "",
    ]
    for header in includes or []:
        if "\n" in header or '"' in header:
            raise ValueError("Invalid include name")
        lines.append("#include %s" % (header if header.startswith("<") else f'"{header}"'))
    if templates:
        lines += ["", "template <{}>".format(", ".join(templates))]
    lines += [
        f"class {class_name}",
        "{",
        " public:",
        "  {}({}) {{}}".format(class_name, ", ".join(constructors)),
        "};",
        "",
    ]
    folder.mkdir(parents=True)
    _write(folder / (class_name + ".hpp"), "\n".join(lines))
    _write(folder / "CMakeLists.txt", CMAKE)
    _write(folder / "README.md", README % {"name": class_name, "description": description})
    _write(folder / ".github/workflows/build.yml", CI_WORKFLOW % "[]")
    return folder
