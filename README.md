# XRobot

LibXR 模块管理与主函数生成工具 / Module manager and main function generator for LibXR

<h1 align="center">
<img src="https://github.com/xrobot-org/XRobot/raw/master/imgs/XRobot.jpeg" width="300">
</h1><br>

[![License](https://img.shields.io/badge/license-Apache--2.0-blue)](https://github.com/xrobot-org/XRobot/blob/master/LICENSE)
[![GitHub Repo](https://img.shields.io/github/stars/xrobot-org/XRobot?style=social)](https://github.com/xrobot-org/XRobot)
[![Documentation](https://img.shields.io/badge/docs-online-brightgreen)](https://xrobot.work/)
[![GitHub Issues](https://img.shields.io/github/issues/xrobot-org/XRobot)](https://github.com/xrobot-org/XRobot/issues)
[![CI/CD - Python Package](https://github.com/xrobot-org/XRobot/actions/workflows/python-publish.yml/badge.svg)](https://github.com/xrobot-org/XRobot/actions/workflows/python-publish.yml)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fxrobot-org%2FXRobot.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fxrobot-org%2FXRobot?ref=badge_shield)

XRobot 是配合 [LibXR](https://github.com/xrobot-org/libxr) 使用的模块管理工具。它负责拉取模块、
把每个模块锁定到具体的提交，再根据 `User/` 下的 YAML 配置生成主函数 `XRobotMain`。

XRobot is the Module manager for [LibXR](https://github.com/xrobot-org/libxr). It fetches
Modules, locks each one to a commit, and generates the main function `XRobotMain` from
the YAML configurations under `User/`.

---

## 🔧 安装 / Installation

需要 Python 3.10 或更高版本。

Requires Python 3.10 or later.

### 使用 pipx 安装 (Install via `pipx`)

Windows

```powershell
python -m pip install --user pipx
python -m pipx ensurepath
python -m pipx install xrobot
# 重新打开终端 / Restart your terminal
```

Linux

```bash
sudo apt install pipx
pipx ensurepath
pipx install xrobot
# 重新打开终端 / Restart your terminal
```

### 使用 pip 安装 (Install via `pip`)

```bash
pip install xrobot
```

### 从源码安装 (Install from source)

```bash
git clone https://github.com/xrobot-org/XRobot.git
cd XRobot
pip install .
```

后两种方式使用 pip，适用于 Windows 和虚拟环境。Debian、Ubuntu 的系统 Python 不接受 pip 直接安装的软件包，
会报 `externally-managed-environment`；在这些系统上使用 pipx，或在虚拟环境中运行 pip。

The last two methods use pip, which suits Windows and virtual environments. The system
Python of Debian and Ubuntu rejects packages installed directly with pip and reports
`externally-managed-environment`; on these systems use pipx, or run pip inside a virtual
environment.

以上三种方式只选其一，不要混用。系统中有多份安装时，命令行实际调用的版本可能与预期不同，而不同版本
生成的代码并不一致。当前使用的版本可通过 `xrobot --version` 查看。

Use only one of these methods. With several installations present, the command line may
run a different version than expected, and different versions generate different code.
`xrobot --version` shows the version in use.

BSP 使用的 XRobot 版本记录在 `Modules/modules.yaml` 的 `xrobot:` 字段中，安装时应与之一致，例如
`pipx install xrobot==1.0.0`。

The XRobot version a BSP uses is recorded in the `xrobot:` field of `Modules/modules.yaml`;
install the same version, e.g. `pipx install xrobot==1.0.0`.

---

## 📚 基本概念 / Concepts

可以在不同板子之间复用的驱动和算法以模块的形式发布，板级初始化保留在各自的 BSP 中。以一块带有
BMI088 IMU 和 LED 的板子为例，各个概念的对应关系如下：

Drivers and algorithms that can be reused across boards are published as Modules, while
board-specific setup remains in each BSP. For a board with a BMI088 IMU and an LED, the
concepts correspond as follows:

| 概念 Concept | 在这个例子里 | In this example |
| --- | --- | --- |
| 模块 Module | `xrobot-org/BMI088`（IMU 驱动）、`xrobot-org/MadgwickAHRS`（姿态解算）、`xrobot-org/BlinkLED`（状态灯），各自是一个 Git 仓库 | `xrobot-org/BMI088` (IMU driver), `xrobot-org/MadgwickAHRS` (attitude estimation), `xrobot-org/BlinkLED` (status LED), one Git repository each |
| BSP | 这块板子的工程：芯片初始化、SPI 和 GPIO 等外设对象、构建脚本 | The board's project: chip setup, peripheral objects such as SPI and GPIO, build scripts |
| 硬件注册 Registration | BSP 通过 `XR_REGISTER(spi1, LibXR::SPI)`、`XR_REGISTER(LED_R, LibXR::GPIO)` 将外设提供给模块 | The BSP provides peripherals to Modules through `XR_REGISTER(spi1, LibXR::SPI)` and `XR_REGISTER(LED_R, LibXR::GPIO)` |
| 入口源文件 Entry source | BSP 中创建外设、注册外设并调用 `XROBOT_MAIN()` 的源文件，例如 `User/app_main.cpp` | The BSP source that creates and registers the peripherals and calls `XROBOT_MAIN()`, e.g. `User/app_main.cpp` |
| 实例 Instance | `imu` 是一个接在 `spi1` 上的 BMI088，`ahrs` 是一个 MadgwickAHRS，`led` 是一个用 `LED_R` 的 BlinkLED | `imu` is a BMI088 on `spi1`, `ahrs` a MadgwickAHRS, `led` a BlinkLED on `LED_R` |
| 配置 Configuration | `User/xrobot.yaml`，按顺序列出 `imu`、`ahrs`、`led` 和它们的参数 | `User/xrobot.yaml`, listing `imu`, `ahrs` and `led` with their parameters, in order |
| 主函数 XRobotMain | 由配置生成的 C++ 函数，由 `XROBOT_MAIN()` 调用，负责创建上述三个对象 | The C++ function generated from the configuration and called by `XROBOT_MAIN()`; it creates the three objects |
| 源 Source | 列出这些模块仓库的 `index.yaml`，默认使用 xrobot-org 的官方源 | An `index.yaml` listing the repositories; the official xrobot-org source is the default |
| lock | `xrobot.lock`，记录每个模块使用的提交 | `xrobot.lock`, recording the commit used for each Module |

---

## 📦 模块与版本 / Modules and Versions

BSP 在 `Modules/modules.yaml` 中列出所需的模块。`xrobot setup` 从源中查找这些模块及其依赖，拉取到
`Modules/` 目录，并将所用的提交记录在 `xrobot.lock` 中。此后的 `setup` 均按 lock 检出，同一个 BSP
在不同机器上得到的模块代码一致；升级模块时运行 `xrobot setup --update`。

A BSP lists the Modules it needs in `Modules/modules.yaml`. `xrobot setup` looks them and
their dependencies up in the sources, fetches them into `Modules/`, and records the commits
in `xrobot.lock`. Later runs check out the locked commits, so the same BSP gets the same
Module code on every machine; `xrobot setup --update` upgrades the Modules.

```yaml
xrobot: 1.0.0
modules:
  - xrobot-org/BMI088@same-or-dev
  - xrobot-org/MadgwickAHRS@same-or-dev
  - xrobot-org/BlinkLED@same-or-dev
```

```bash
$ xrobot setup
Resolved 3 Module commits
Checked 1 config; generated User/xrobot_main.hpp for User/xrobot.yaml

$ xrobot source search BMI
xrobot-org/BMI088 [module] https://github.com/xrobot-org/BMI088.git
xrobot-org/BMI270 [module] https://github.com/xrobot-org/BMI270.git
```

`@same-or-dev` 表示优先使用与 BSP 同名的模块分支，不存在时使用 `dev`。依赖链上的 `@same`、
`@same-or-dev` 请求和显式 tag、提交号都沿用最初的上下文分支：每一层各自查找同名分支、
找不到再退回 `dev`，中间层退回 `dev` 不改变下一层跟随的分支。

`@same-or-dev` selects the Module branch with the same name as the BSP's branch, falling
back to `dev`. Along the dependency chain, `@same` and `@same-or-dev` requests and explicit
tags or commits all keep the original context branch: every layer looks for its own branch
of that name and falls back to `dev`, and a middle layer's fallback does not change the
branch the next layer follows.

---

## 🧩 从配置到代码 / From Configuration to Code

配置中的每一项是一个实例，参数与模块构造函数的参数对应，外设以 BSP 注册时的名称引用：

Each entry of a configuration is an instance. Its parameters correspond to the Module's
constructor parameters, and peripherals are referenced by the names the BSP registered:

```yaml
modules:
  - module: xrobot-org/BlinkLED
    id: led
    args:
      - led: LED_R
      - blink_cycle: 250
settings:
  monitor_sleep_ms: 1000
```

指针参数的裸名由生成器自动取地址；带引号的 `'&名字'` 仍然接受，按写出的地址传递；可选的依赖不使用时写
`nullptr`。不带引号的 `&名字` 是 YAML 锚点、值为空，报错会建议去掉 `&` 只写名字。

The generator takes the address of a bare name for a pointer parameter automatically; the quoted
`'&name'` is still accepted and passes the address as written; write `nullptr` to leave an optional
dependency unused. Without quotes, `&name` is a YAML anchor with an empty value, and the error
suggests dropping the `&` and writing the bare name.

`LED_R` 由 BSP 在入口源文件中创建并注册，入口源文件最后调用 `XROBOT_MAIN()`：

`LED_R` is created and registered by the BSP in its entry source, which ends by calling
`XROBOT_MAIN()`:

```cpp
// User/app_main.cpp（节选 / excerpt）
static STM32GPIO LED_R(LED_R_GPIO_Port, LED_R_Pin);
static STM32SPI spi1(&hspi1, spi1_rx_buf, spi1_tx_buf, 3);
// ...
XR_REGISTER(LED_R, LibXR::GPIO);
XR_REGISTER(spi1, LibXR::SPI);
// ...
XROBOT_MAIN();
```

`xrobot gen` 由配置生成普通的 C++ 代码，对象按配置顺序静态创建，排版与 LibXR 的 `.clang-format`
一致：

`xrobot gen` generates plain C++ code from the configuration, creating the objects
statically in configuration order. The layout follows LibXR's `.clang-format`:

```cpp
// User/xrobot_main.hpp（节选 / excerpt）
// Generated by `xrobot gen` from User/xrobot.yaml; do not edit by hand.
#pragma once

#include "BlinkLED.hpp"
#include "libxr.hpp"
#include "thread.hpp"

[[noreturn]] static inline void XRobotMain(LibXR::GPIO& LED_R)
{
  // led: xrobot-org/BlinkLED (User/xrobot.yaml:2)
#line 2 "User/xrobot.yaml"
  static BlinkLED led(LED_R, 250);
#line 14 "User/xrobot_main.hpp"

  for (;;)
  {
    LibXR::Thread::Sleep(1000);
  }
}
// ...
```

每个实例前的注释写明模块和 YAML 中的行号；`#line` 指令使实例的每行代码对应到它来自的 YAML 行，编译错误
因此定位到 YAML 中的那一行，路径相对 BSP 根目录，编辑器可以直接打开。`#line` 只写在编译器自己数出的
行号对不上的地方，`xrobot gen --no-line-directives` 可以省略它们。传给引用、`std::initializer_list` 或结构体参数的值
写成紧邻实例之前的 `static const <类型> xr_<实例>_<参数> = {…};`，与实例同为静态存储期。配置或
模块修改后若未重新生成，构建会中止，并提示需要执行的命令：

A comment before each instance names its Module and the line of the YAML. The `#line`
directives map every line of the instance's code to the YAML line it comes from, so a compiler
error refers to that line, with a path relative to the BSP root that an editor opens directly.
A `#line` is written only where the line the compiler counts by itself would differ (`xrobot gen
--no-line-directives` leaves the directives out). Values for reference, `std::initializer_list`
and struct parameters are written as `static const <type> xr_<instance>_<parameter> = {...};`
right before the instance and, like the instance, have static storage duration. If a
configuration or Module has changed without regenerating the code, the build stops and
reports the command to run:

```text
[XRobot] <BSP>/User/xrobot_main.hpp is stale: its inputs changed after it was generated.
Run `xrobot gen -c User/xrobot.yaml` in <BSP> before building.
```

---

## 🔀 一块板子，多份配置 / One Board, Several Configurations

同一个 BSP 可以包含多份配置，例如用于调试的精简配置和完整功能的配置。`xrobot gen -c` 选择要生成的
配置，之后的 `xrobot gen` 和 `xrobot setup` 沿用该选择：

A BSP can contain several configurations, for example a minimal one for debugging and a
full one. `xrobot gen -c` selects the configuration to generate, and subsequent
`xrobot gen` and `xrobot setup` runs keep that selection:

```bash
$ xrobot gen -c User/configs/debug.yaml
Generated User/xrobot_main.hpp for User/configs/debug.yaml

$ xrobot gen
User/xrobot_main.hpp for User/configs/debug.yaml is unchanged
```

---

## ✍️ 模块结构 / Module Structure

模块是一个普通的 C++ 类。以下节选自 BMI088 的头文件：注释中的 manifest 记录模块说明和所依赖的其他
模块（BMI088 没有依赖）；构造函数先列出模块依赖的对象（外设和 LibXR 服务），其后是带默认值的配置参数。

A Module is an ordinary C++ class. The excerpt below is from the BMI088 header: the manifest
comment records the description and the Modules it depends on (BMI088 has none); the
constructor lists the objects the Module depends on (peripherals and LibXR services) first,
followed by configuration parameters with defaults.

```cpp
// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: 博世 BMI088 6 轴 IMU（SPI）驱动模块 / Driver Module for the Bosch BMI088 6-axis IMU over SPI
depends: []
=== END MANIFEST === */
// clang-format on

// ...

class BMI088
{
 public:
  BMI088(LibXR::GPIO& accl_cs, LibXR::GPIO& gyro_cs, LibXR::GPIO& gyro_int,
         LibXR::SPI& spi, LibXR::PWM& heater_pwm, LibXR::Database& database,
         LibXR::RamFS& ramfs, const Param& param = {...});
};
```

`xrobot new-module` 可生成新模块的骨架，包括头文件、`CMakeLists.txt`、README 和 CI 配置。各模块的 CI
均调用本仓库的 `.github/workflows/module-ci.yml`。

`xrobot new-module` generates the skeleton of a new Module: header, `CMakeLists.txt`, README
and CI configuration. The CI of every Module calls `.github/workflows/module-ci.yml` in this
repository.

STM32 BSP 的 CI 调用 `.github/workflows/bsp-stm32-ci.yml`，BSP 只需写出工程名和要构建的配置：

The CI of an STM32 BSP calls `.github/workflows/bsp-stm32-ci.yml`, and the BSP only names its
project and the configurations to build:

```yaml
name: build
on:
  push: {branches: [master, dev], tags: ['v*']}
  pull_request: {branches: [master, dev]}
  release: {types: [published]}
  workflow_dispatch:
jobs:
  build:
    permissions: {contents: write}   # 把固件附到发布 / attach the firmware to releases
    uses: xrobot-org/XRobot/.github/workflows/bsp-stm32-ci.yml@v1
    with:
      project: DevC
      config-dir: User/configs
      configs: |
        default
        debug
        full
```

共享流程按固定的版本安装工具、检出锁定的模块并逐份配置构建，打 `v*` tag 或发布 Release 时上传固件。
检查项、可选输入和发布的文件见文档 [BSP CI](https://xrobot.work/docs/proj_man/proj-man-ci#bsp-ci)。

The shared workflow installs the pinned tool versions, checks out the locked Modules, builds
every configuration, and uploads the firmware for a `v*` tag or a published Release. The
checks, the optional inputs and the published files are described in
[BSP CI](https://xrobot.work/en/docs/proj_man/proj-man-ci#bsp-ci).

---

## 🚀 命令一览 / Commands

| 命令 Command | 说明 | Description |
| --- | --- | --- |
| `xrobot init` | 创建 BSP 文件（含 `.gitignore` 和 `.gitattributes` 条目） | Create the BSP files, with `.gitignore` and `.gitattributes` entries |
| `xrobot setup` | 拉取模块，检查所有配置，生成主函数 | Fetch Modules, check all configurations, generate the main function |
| `xrobot gen [-c CONFIG] [--no-line-directives]` | 生成主函数，可切换配置，可省略 `#line` | Generate the main function, optionally for another configuration, optionally without `#line` |
| `xrobot sync` | 按默认值补入模块新增的字段和参数，删除已移除的参数 | Add new fields and parameters with defaults and drop removed ones |
| `xrobot format [--check]` | 整理配置格式 | Format the configurations |
| `xrobot instance add\|set\|remove\|rename` | 编辑实例 | Edit instances |
| `xrobot module add\|remove\|show` | 增删模块，查看模块接口 | Add or remove Modules, show a Module's interface |
| `xrobot new-module NAME` | 新建模块 | Create a Module |
| `xrobot check-module MODULE` | 生成模块 CI 用的编译检查 | Write the compile check used by Module CI |
| `xrobot source ...` | 查询和编辑源 | Query and edit sources |
| `xrobot describe` | 以 JSON 输出 BSP 状态（VS Code 扩展使用） | Print the BSP state as JSON (used by the VS Code extension) |

入门教程和完整说明见文档 / Tutorials and reference: <https://xrobot.work/docs/proj_man> ·
<https://xrobot.work/en/docs/proj_man>

---

## 🧪 测试 / Tests

```bash
python -m unittest discover -s tests -v
```

---

## 📖 更多信息 / More Information

- [GitHub Repository](https://github.com/xrobot-org/XRobot)
- [Documentation](https://xrobot.work)
- [Issue Tracker](https://github.com/xrobot-org/XRobot/issues)
- [发版 / Releases](https://github.com/xrobot-org/XRobot/blob/master/RELEASE.md)
