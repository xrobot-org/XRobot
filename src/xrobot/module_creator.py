"""新建模块：头文件（manifest 与构造函数）、CMakeLists.txt、README 和调用共享模块 CI 的工作流。
Create a Module: its header (manifest and constructor), CMakeLists.txt, README and the
workflow that calls the shared Module CI.

写文件前，头文件先按 setup 和 instance add 的规则解析和检查，出错时不创建任何文件。
Before anything is written the header is parsed and checked with the rules of setup and
instance add, so an error creates nothing.
"""

from __future__ import annotations

import json
import re
from pathlib import Path

import yaml

from xrobot.config import identifier_problem
from xrobot.config_edit import instance_item, instance_text
from xrobot.constructor_model import compliant_constructors, enrich_interface, is_dependency
from xrobot.lock import request
from xrobot.source_syntax import extract_interface

# libxr.hpp 包含 LibXR 的核心类型，但不包含 libxr/src/driver 下的硬件接口；构造参数用到
# 硬件接口时单独包含它的头文件。
# libxr.hpp includes LibXR's core types but not the hardware interfaces under
# libxr/src/driver; a constructor parameter that uses one includes its header.
DRIVER_HEADERS = {
    "ADC": "adc.hpp",
    "CAN": "can.hpp",
    "FDCAN": "can.hpp",
    "DAC": "dac.hpp",
    "Flash": "flash.hpp",
    "GPIO": "gpio.hpp",
    "I2C": "i2c.hpp",
    "PowerManager": "power.hpp",
    "PWM": "pwm.hpp",
    "SPI": "spi.hpp",
    "UART": "uart.hpp",
    "Watchdog": "watchdog.hpp",
}

CMAKE = """target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
file(GLOB MODULE_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
     "${CMAKE_CURRENT_LIST_DIR}/*.cc"
     "${CMAKE_CURRENT_LIST_DIR}/*.cxx"
     "${CMAKE_CURRENT_LIST_DIR}/*.c")
target_sources(xr PRIVATE ${MODULE_SOURCES})
"""


def ci_workflow(template_args: list[str]) -> str:
    """调用共享模块 CI 的工作流；template_args 是构造探针使用的模板实参。
    The workflow that calls the shared Module CI; template_args are the template arguments
    of the constructor probe.
    """
    values = json.dumps(template_args, ensure_ascii=False).replace("'", "''")
    return f"""name: Module CI

on:
  push:
  pull_request:
  workflow_dispatch:

jobs:
  build:
    uses: xrobot-org/XRobot/.github/workflows/module-ci.yml@v1
    with:
      template-args: '{values}'
"""


def _include_lines(declarations: list[str], extra: list[str]) -> list[str]:
    """头文件的 #include 行：尖括号的一组在前，引号的一组在后，各自排序。
    The #include lines of the header: the angle-bracket group first, then the quoted
    group, each sorted.

    引号的一组总有 libxr.hpp，另有声明中用到的 LibXR 硬件接口的头文件。
    The quoted group always has libxr.hpp, plus the headers of the LibXR hardware
    interfaces the declarations use.

    Raises:
        ValueError: 头文件名含换行或引号，或尖括号不成对。
            A header name contains a line break or a quote, or unbalanced angle brackets.
    """
    quoted = {"libxr.hpp"}
    for name in re.findall(r"\bLibXR::(\w+)", "\n".join(declarations)):
        if name in DRIVER_HEADERS:
            quoted.add(DRIVER_HEADERS[name])
    system = set()
    for header in extra:
        if not header or "\n" in header or '"' in header:
            raise ValueError(f"Invalid include name: {header!r}")
        if header.startswith("<") != header.endswith(">"):
            raise ValueError(f"Invalid include name: {header!r}")
        (system if header.startswith("<") else quoted).add(header)
    lines = [f"#include {header}" for header in sorted(system)]
    if lines:
        lines.append("")
    return lines + [f'#include "{header}"' for header in sorted(quoted)]


def _header(
    class_name: str,
    description: str,
    depends: list[dict],
    constructors: list[str],
    templates: list[str],
    includes: list[str],
) -> str:
    """模块头文件的文本：manifest、#include 和只有一个空构造函数的类。
    The text of the Module header: manifest, #include lines and a class with one empty
    constructor.
    """
    manifest = yaml.safe_dump(
        {"module_description": description, "depends": depends},
        sort_keys=False,
        allow_unicode=True,
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
        *_include_lines(constructors + templates, includes),
        "",
    ]
    if templates:
        lines.append(f"template <{', '.join(templates)}>")
    lines += [
        f"class {class_name}",
        "{",
        " public:",
        f"  {class_name}({', '.join(constructors)}) {{}}",
        "};",
        "",
    ]
    return "\n".join(lines)


def _interface(text: str, class_name: str, constructors: list[str], templates: list[str]) -> dict:
    """按 setup 和 instance add 的规则解析新头文件的构造接口。
    Parse the constructor interface of the new header with the rules of setup and
    instance add.

    Raises:
        ValueError: 某个声明不是恰好一个参数，参数没有名字或默认值为空，或依赖参数排在
            带默认值的参数之后。
            A declaration is not exactly one parameter, a parameter has no name or an
            empty default, or a dependency follows a parameter with a default.
    """
    interface = enrich_interface(text, extract_interface(text, class_name))
    arguments = [p["declaration"] for p in interface["constructors"][0]["arguments"]]
    parameters = [p["declaration"] for p in interface["template_parameters"]]
    if len(interface["constructors"]) != 1 or arguments != constructors or parameters != templates:
        raise ValueError("each --constructor and --template must be one parameter declaration")
    compliant_constructors(interface)
    return interface


def _ci_template_args(interface: dict, values: list[str]) -> list[str]:
    """模块 CI 使用的模板实参；给出的值按位置对应，其余参数必须有默认值。
    The template arguments the Module CI uses; the values given match by position and the
    remaining parameters must have defaults.

    Raises:
        ValueError: 值多于模板参数，或有参数既没有给值也没有默认值。
            More values than template parameters, or a parameter with neither a value nor
            a default.
    """
    parameters = interface["template_parameters"]
    if len(values) > len(parameters):
        count = f"{len(parameters)} template parameter" + ("" if len(parameters) == 1 else "s")
        raise ValueError(
            f"{interface['name']} has {count} but {len(values)} --template-arg values were given"
        )
    missing = [p["name"] for p in parameters[len(values) :] if p["default"] is None]
    if missing:
        raise ValueError(
            f"template parameter {', '.join(missing)} of {interface['name']} has no default; "
            "give the value the Module CI compiles with --template-arg"
        )
    return list(values)


def _readme(
    class_name: str, description: str, interface: dict, depends: list[dict], templates: list[str]
) -> str:
    """按模块 README 模板写出的 README；工具不知道的内容以 HTML 注释留给作者。
    The README laid out by the Module README template; what the tool cannot know is left to
    the author as HTML comments.
    """
    ctor = interface["constructors"][0]["arguments"]
    dependencies = [p for p in ctor if is_dependency(p)]
    values = [p for p in ctor if not is_dependency(p)]
    hardware = [
        (p["name"], "LibXR::" + name)
        for p in dependencies
        for name in re.findall(r"\bLibXR::(\w+)", p["type"])
        if name in DRIVER_HEADERS
    ]

    def listed(items: list[str]) -> str:
        """列出的各项；没有时为“无 / None”。
        The listed items, or the bilingual word for none when there are none.
        """
        return "\n".join(items) if items else "无 / None"

    signature = f"{class_name}({', '.join(p['declaration'] for p in ctor)});"
    constructor = ["```cpp", signature, "```", ""]
    if interface["template_parameters"]:
        constructor += ["模板参数 / Template parameters:", ""]
        constructor += [f"- `{p['declaration']}`" for p in interface["template_parameters"]]
        constructor.append("")
    if dependencies:
        constructor += ["依赖参数 / Dependencies:", ""]
        constructor += [f"- `{p['declaration']}` <!-- 用途 / purpose -->" for p in dependencies]
        constructor.append("")
    if values:
        constructor += ["配置参数 / Configuration:", ""]
        constructor += [f"- `{p['declaration']}` <!-- 用途 / purpose -->" for p in values]
        constructor.append("")
    example = instance_text(
        instance_item(class_name, class_name.lower() + "_0", class_name, interface, None, templates)
    )
    purpose = [description, ""] if description else []
    return "\n".join(
        [
            f"# {class_name}",
            "",
            "## 1. 模块作用",
            "",
            *purpose,
            "<!-- 模块做什么、何时运行：中文一段，英文一段。",
            "     What the Module does and when it runs: a Chinese paragraph, then an English one. -->",
            "",
            "## 2. 构造接口",
            "",
            *constructor,
            "## 3. Topic",
            "",
            "<!-- 发布和订阅的 Topic：名称、类型和用途；没有时写“无 / None”。",
            "     Topics published and subscribed: name, type and purpose; None when there are none. -->",
            "",
            "## 4. 配置示例",
            "",
            "`xrobot instance add` 按下面的形式写出实例。依赖参数留空，填为 BSP 中用 `XR_REGISTER` "
            "注册的名字或前面实例的 id。",
            "",
            "`xrobot instance add` writes the instance in the form below. The dependencies are "
            "left empty and take names the BSP registers with `XR_REGISTER` or ids of earlier "
            "instances.",
            "",
            "```yaml",
            example.rstrip("\n"),
            "```",
            "",
            "## 5. 依赖与硬件",
            "",
            "模块依赖 / Module dependencies:",
            "",
            listed([f"- `{d['id']}@{d['ref']}`" for d in depends]),
            "",
            "硬件 / Hardware:",
            "",
            listed([f"- `{name}`: `{cpp_type}`" for name, cpp_type in hardware]),
            "",
        ]
    )


def create_module(
    class_name: str,
    description: str = "",
    constructor_args: list[str] | None = None,
    template_args: list[str] | None = None,
    depends: list[str] | None = None,
    output_dir: str | Path = ".",
    includes: list[str] | None = None,
    ci_template_args: list[str] | None = None,
) -> Path:
    """在 output_dir 下新建模块文件夹 class_name。
    Create the Module folder class_name under output_dir.

    Args:
        constructor_args: 构造参数声明，依赖在前，带默认值的配置在后。
            Constructor parameter declarations, dependencies first, then configuration
            with defaults.
        template_args: 模板参数声明。
            Template parameter declarations.
        depends: 依赖的模块，写法为 owner/Repo[@ref]。
            Modules this one depends on, as owner/Repo[@ref].
        includes: 另外包含的头文件；<...> 为系统头文件，其余加引号。
            Further headers to include; <...> is a system header, the rest are quoted.
        ci_template_args: 模块 CI 编译构造探针时使用的模板实参。
            The template arguments the Module CI compiles the constructor probe with.

    Returns:
        新模块文件夹。
        The new Module folder.

    Raises:
        ValueError: 名字、声明、依赖或模板实参不合要求，或文件夹已存在；此时不创建任何文件。
            The name, a declaration, a dependency or the template arguments are invalid,
            or the folder exists; nothing is created then.
    """
    problem = identifier_problem(class_name)
    if problem:
        raise ValueError(f"Module name {class_name} {problem}")
    folder = Path(output_dir) / class_name
    if folder.exists():
        raise ValueError(f"Refusing to overwrite existing module: {folder}")
    constructors = [item.strip() for item in constructor_args or []]
    templates = [item.strip() for item in template_args or []]
    if not all(constructors + templates):
        raise ValueError("--constructor and --template need a C++ parameter declaration")
    entries = []
    for dependency in depends or []:
        parsed = request(dependency, canonical=True)
        entries.append({"id": parsed["id"], "ref": parsed["ref"] or "same-or-dev"})
    header = _header(class_name, description, entries, constructors, templates, includes or [])
    try:
        interface = _interface(header, class_name, constructors, templates)
    except ValueError as error:
        raise ValueError(f"{class_name}.hpp would not be a valid Module header: {error}") from None
    ci_args = _ci_template_args(interface, list(ci_template_args or []))

    files = {
        class_name + ".hpp": header,
        "CMakeLists.txt": CMAKE,
        "README.md": _readme(class_name, description, interface, entries, ci_args),
        ".github/workflows/build.yml": ci_workflow(ci_args),
    }
    for name, text in files.items():
        path = folder / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode("utf-8"))
    return folder
