"""新建模块骨架（xrobot.module_creator）。
Creating a Module skeleton (xrobot.module_creator).
"""

import yaml
from fixtures import BspTestCase, CliMixin

from xrobot.module_parser import parse_manifest_from_header, source_interface


class NewModule(CliMixin, BspTestCase):
    """new-module 创建的文件和对输入的检查。
    The files new-module creates and its checks of the input.
    """

    def test_new_module_skeleton(self):
        out, _ = self.ok(
            "new-module",
            "Blink",
            "--desc",
            "Blinks a pin",
            "--constructor",
            "LibXR::GPIO& gpio",
            "--constructor",
            "int period_ms = 500",
            "--depends",
            "team/Timer",
            "--depends",
            "team/Log@v1",
            "--include",
            "<array>",
            "--include",
            "blink_types.hpp",
            "--out",
            self.tmp / "out",
        )
        folder = self.tmp / "out/Blink"
        self.assertEqual(out.strip(), f"Created {folder}")
        manifest = parse_manifest_from_header(folder / "Blink.hpp")
        self.assertEqual(manifest.description, "Blinks a pin")
        self.assertEqual(
            manifest.depends,
            [{"id": "team/Timer", "ref": "same-or-dev"}, {"id": "team/Log", "ref": "v1"}],
        )
        interface = source_interface(folder / "Blink.hpp")
        self.assertEqual(
            [p["declaration"] for p in interface["constructors"][0]["arguments"]],
            ["LibXR::GPIO& gpio", "int period_ms = 500"],
        )
        # libxr.hpp 总被包含；GPIO 是硬件接口，另外包含 gpio.hpp。
        # libxr.hpp is always included; GPIO is a hardware interface, so gpio.hpp as well.
        self.assertEqual(
            (folder / "Blink.hpp").read_text(encoding="utf-8"),
            """#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: Blinks a pin
depends:
- id: team/Timer
  ref: same-or-dev
- id: team/Log
  ref: v1
=== END MANIFEST === */
// clang-format on

#include <array>

#include "blink_types.hpp"
#include "gpio.hpp"
#include "libxr.hpp"

class Blink
{
 public:
  Blink(LibXR::GPIO& gpio, int period_ms = 500) {}
};
""",
        )
        self.assertIn(
            'target_include_directories(xr PUBLIC "${CMAKE_CURRENT_LIST_DIR}")',
            (folder / "CMakeLists.txt").read_text(encoding="utf-8"),
        )
        workflow = yaml.safe_load(
            (folder / ".github/workflows/build.yml").read_text(encoding="utf-8")
        )
        job = workflow["jobs"]["build"]
        self.assertEqual(job["uses"], "xrobot-org/XRobot/.github/workflows/module-ci.yml@v1")
        self.assertEqual(job["with"], {"template-args": "[]"})
        self.assertEqual(
            (folder / "README.md").read_text(encoding="utf-8"),
            """# Blink

## 1. 模块作用

Blinks a pin

<!-- 模块做什么、何时运行：中文一段，英文一段。
     What the Module does and when it runs: a Chinese paragraph, then an English one. -->

## 2. 构造接口

```cpp
Blink(LibXR::GPIO& gpio, int period_ms = 500);
```

依赖参数 / Dependencies:

- `LibXR::GPIO& gpio` <!-- 用途 / purpose -->

配置参数 / Configuration:

- `int period_ms = 500` <!-- 用途 / purpose -->

## 3. Topic

<!-- 发布和订阅的 Topic：名称、类型和用途；没有时写“无 / None”。
     Topics published and subscribed: name, type and purpose; None when there are none. -->

## 4. 配置示例

`xrobot instance add` 按下面的形式写出实例。依赖参数留空，填为 BSP 中用 `XR_REGISTER` \
注册的名字或前面实例的 id。

`xrobot instance add` writes the instance in the form below. The dependencies are left empty \
and take names the BSP registers with `XR_REGISTER` or ids of earlier instances.

```yaml
modules:
  - module: Blink
    id: blink_0
    args:
      - gpio:
      - period_ms: 500
```

## 5. 依赖与硬件

模块依赖 / Module dependencies:

- `team/Timer@same-or-dev`
- `team/Log@v1`

硬件 / Hardware:

- `gpio`: `LibXR::GPIO`
""",
        )

    def test_new_module_template_arguments_for_ci(self):
        out = self.tmp / "out"
        self.fails(
            "new-module",
            "Filter",
            "--template",
            "int N",
            "--out",
            out,
            message="template parameter N of Filter has no default; give the value the Module CI "
            "compiles with --template-arg",
        )
        self.fails(
            "new-module",
            "Filter",
            "--template",
            "int N",
            *("--template-arg", "3", "--template-arg", "4"),
            "--out",
            out,
            message="Filter has 1 template parameter but 2 --template-arg values were given",
        )
        self.assertFalse((out / "Filter").exists())
        self.ok(
            "new-module",
            "Filter",
            *("--template", "int N", "--template", "typename T = float"),
            *("--template-arg", "3"),
            *("--constructor", "T gain = T(1)"),
            "--out",
            out,
        )
        workflow = yaml.safe_load(
            (out / "Filter/.github/workflows/build.yml").read_text(encoding="utf-8")
        )
        self.assertEqual(workflow["jobs"]["build"]["with"], {"template-args": '["3"]'})
        # README 的配置示例用 CI 的模板实参，参数也一并写出。
        # The README example uses the CI template arguments, so the arguments are written.
        self.assertIn(
            "    template_args:\n      - 3\n      - float\n    args:\n      - gain: float(1)\n",
            (out / "Filter/README.md").read_text(encoding="utf-8"),
        )

    def test_new_module_refuses_bad_input_without_creating_anything(self):
        out = self.tmp / "out"
        self.fails(
            "new-module",
            "Blink",
            "--depends",
            "Timer",
            "--out",
            out,
            message="Expected canonical owner/repo: 'Timer'",
        )
        self.fails(
            "new-module",
            "1Blink",
            "--out",
            out,
            message="Module name 1Blink is not a C++ identifier",
        )
        self.fails(
            "new-module", "class", "--out", out, message="Module name class is a C++ keyword"
        )
        invalid = "Blink.hpp would not be a valid Module header: "
        for declarations, problem in (
            (["LibXR::GPIO&"], "Constructor parameters must have explicit names: LibXR::GPIO&"),
            (["int x = "], "Missing default value after '=': int x ="),
            (
                ["int x = 1", "LibXR::GPIO& led"],
                "Blink: no compliant constructor; line \\d+: led: dependency without a "
                "default appears after value configuration",
            ),
            (["int x) {} void f(int y"], "each --constructor and --template must be one"),
        ):
            options = [a for d in declarations for a in ("--constructor", d)]
            self.fails("new-module", "Blink", *options, "--out", out, pattern=invalid + problem)
        self.fails(
            "new-module",
            "Blink",
            "--include",
            "<array",
            "--out",
            out,
            message="Invalid include name: '<array'",
        )
        self.assertFalse(out.exists())
        self.ok("new-module", "Blink", "--out", out)
        self.fails(
            "new-module",
            "Blink",
            "--out",
            out,
            message=f"Refusing to overwrite existing module: {out / 'Blink'}",
        )
