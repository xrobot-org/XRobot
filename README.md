
# XRobot PkgKit 模块包管理与代码入口生成工具 / Package Management and Entry Point Generation Toolkit

<h1 align="center">
<img src="https://github.com/Jiu-xiao/LibXR_CppCodeGenerator/raw/main/imgs/XRobot.jpeg" width="300">
</h1><br>

[![License](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE)
[![GitHub Repo](https://img.shields.io/github/stars/xrobot-org/XRobot?style=social)](https://github.com/xrobot-org/XRobot)
[![Documentation](https://img.shields.io/badge/docs-online-brightgreen)](https://xrobot-org.github.io/)
[![GitHub Issues](https://img.shields.io/github/issues/xrobot-org/XRobot)](https://github.com/xrobot-org/XRobot/issues)
[![CI/CD - Python Package](https://github.com/xrobot-org/XRobot/actions/workflows/python-publish.yml/badge.svg)](https://github.com/xrobot-org/XRobot/actions/workflows/python-publish.yml)
[![FOSSA Status](https://app.fossa.com/api/projects/git%2Bgithub.com%2Fxrobot-org%2FXRobot.svg?type=shield)](https://app.fossa.com/projects/git%2Bgithub.com%2Fxrobot-org%2FXRobot?ref=badge_shield)

XRobot 是一套为嵌入式系统（如 STM32）设计的自动化代码生成工具，配合模块化硬件抽象层 LibXR 使用，支持模块仓库管理、构造参数配置、C++ 主函数生成等任务。  
XRobot is a suite of automated code generation tools for embedded systems (e.g., STM32), designed to work with the modular hardware abstraction layer LibXR. It supports tasks such as module repository management, constructor configuration, and C++ main function generation.

---

## 🔧 安装 / Installation

### 使用pipx安装 (Install via `pipx`)

windows

```ps
python -m pip install --user pipx
python -m pipx ensurepath
pipx install xrobot
pipx ensurepath
# Restart your terminal
```

linux

```bash
sudo apt install pipx
pipx install xrobot
pipx ensurepath
# Restart your terminal
```

### 使用pip安装 (Install via `pip`)

```bash
pip install xrobot
```

### 从源码安装 (Install from source)

Or install from source:

```bash
git clone https://github.com/xrobot-org/XRobot.git
cd XRobot
pip install .
```

---

## 🧩 功能总览 / Features Overview

- **模块仓库拉取与同步**  
  自动拉取并同步模块仓库，支持递归依赖项解析与版本一致性检查。  
  *Clone and update module repositories with recursive dependency resolution and version consistency checks.*

- **模块构造参数提取与配置**  
  自动解析模块头文件中的构造参数与模板参数，生成 YAML 配置文件并支持手动调整。  
  *Extract constructor and template arguments from module headers, generating adjustable YAML configuration files.*

- **自动生成 `XRobotMain()` 主入口函数**  
  根据模块配置文件自动生成 C++ 主入口函数，支持多模块、多实例和嵌套参数结构。  
  *Auto-generate `XRobotMain()` entry function from module configuration, supporting multiple modules, instances, and nested argument structures.*

- **模块头文件中的 manifest 解析器**  
  支持详细解析模块清单（manifest），提取模块描述、依赖关系、构造参数与模板参数。  
  *Comprehensive parser for module manifests from header files, extracting descriptions, dependencies, constructor, and template arguments.*

- **模块快速生成器（含 CMake 和 README）**  
  一键快速生成符合标准化结构的模块目录，自动包含模块头文件、README 文档及 CMakeLists 构建文件。  
  *Rapid generation of standardized module directories with automatic creation of headers, README, and CMakeLists files.*

- **支持本地或远程 YAML 配置**  
  支持加载本地或远程的 YAML 配置文件，统一管理模块仓库和实例配置，便于跨环境使用。  
  *Support for loading local or remote YAML configuration files, enabling unified module repository and instance management across environments.*

---

## 🚀 命令行工具 / CLI Tools

以下命令在安装后可直接调用：

The following commands can be run after installation:

| 命令 Command        | 说明                             | Description                                        |
| ------------------- | -------------------------------- | -------------------------------------------------- |
| `xrobot_gen_main`   | 自动生成 C++ 主函数              | Generate main C++ entry source file                |
| `xrobot_mod_parser` | 解析模块并打印 manifest 信息     | Parse and show module manifest                     |
| `xrobot_create_mod` | 快速创建标准化模块目录           | Create a new module folder & header                |
| `xrobot_init_mod`   | 拉取并递归同步所有模块仓库       | Clone and recursively sync all module repos        |
| `xrobot_setup`      | 一键初始化工作区和生成主函数     | One-click workspace setup & main function generate |
| `xrobot_add_mod`    | 添加模块仓库或追加模块实例到配置 | Add repo or append module instance config          |
| `xrobot_src_man`    | 多源模块仓库管理与索引工具       | Multi-source module repository management utility  |

---

### `xrobot_add_mod`

该工具根据输入目标（Git 仓库 `namespace/ModuleName[@version]` 或本地模块名）自动判断操作类型，分别将模块仓库记录写入 `Modules/modules.yaml`，或将模块实例信息追加至 `User/xrobot.yaml`。

This tool automatically detects whether the input is a repo (e.g. `namespace/ModuleName[@version]`) or a local module name, and writes to:

- `Modules/modules.yaml` for module repositories  
- `User/xrobot.yaml` for module instance configurations

#### 🚀 使用方法 / Usage

```bash
# 添加模块仓库（写入 Modules/modules.yaml）
xrobot_add_mod xrobot-org/BlinkLED@master

# 追加模块实例（写入 User/xrobot.yaml）
xrobot_add_mod BlinkLED
```

#### 🎛️ 命令行参数 / Command-Line Arguments

- `target`  
  位置参数。模块仓库 ID（如 `namespace/ModuleName[@version]`），或本地模块名。  
  Positional argument: Module repo ID (e.g., `namespace/ModuleName[@version]`) or local module name.

- `--config`, `-c`  
  可选，指定 YAML 配置文件路径。添加仓库时默认为 `Modules/modules.yaml`，添加实例时默认为 `User/xrobot.yaml`。  
  Optional config file path. Defaults to `Modules/modules.yaml` for repos or `User/xrobot.yaml` for instances.

- `--instance-id`  
  可选，手动指定模块实例 ID，若不指定则自动编号（如 `BlinkLED_0`, `BlinkLED_1`）。  
  Optional: Manually set module instance id. If omitted, auto-increment ids like `BlinkLED_0`, `BlinkLED_1` will be used.

#### 📤 输出结果 / Output

- **添加模块仓库（Add module repo）**  
  会将如下内容（纯字符串）添加到 `Modules/modules.yaml`：  
  It will add the following entry to `Modules/modules.yaml`:

  ```yaml
  modules:
    - xrobot-org/BlinkLED@main
  ```

- **追加模块实例（Append module instance）**  
  自动读取该模块的 manifest，提取构造参数和模板参数，自动分配唯一 id，并将如下内容追加到 `User/xrobot.yaml`：  
  It reads the module manifest, extracts constructor/template args, auto-assigns unique id, and appends the following to `User/xrobot.yaml`:

  ```yaml
  modules:
    - id: BlinkLED_0
      name: BlinkLED
      constructor_args:
        blink_cycle: 250
  ```

  如有模板参数，则格式如下：

  ```yaml
  modules:
    - id: MyModule_0
      name: MyModule
      constructor_args:
        foo: bar
      template_args:
        T: int
  ```

#### 示例 / Example

MODULES/BlinkLED/BlinkLED.hpp

```cpp
/* === MODULE MANIFEST ===
module_name: BlinkLED
constructor_args:
  - blink_cycle: 250
=== END MANIFEST === */
```

modules.yaml

```yaml
modules:
  - xrobot-org/BlinkLED@main
  - xrobot-org/OtherModule
```

User/xrobot.yaml

```yaml
modules:
  - id: BlinkLED_0
    name: BlinkLED
    constructor_args:
      blink_cycle: 250
```

---

### `xrobot_init_mod`

该工具用于根据模块配置文件（本地或远程 YAML）自动克隆或更新模块仓库，默认拉取至 `Modules/` 目录。  
This tool clones or updates module repositories based on a YAML config file (local or remote), and stores them under the `Modules/` directory.

#### 🚀 使用方法 / Usage

```bash
# 使用默认配置文件 modules.yaml 并拉取到 Modules/
# Pull modules from modules.yaml to Modules/
xrobot_init_mod

# 指定配置文件和目录
# Specify config file and directory
xrobot_init_mod --config my_config.yaml --directory MyModules
```

#### 🎛️ 命令行参数 / Command-Line Arguments

- `--config`, `-c`  
  指定模块配置文件路径或 URL。默认为 `Modules/modules.yaml`。  
  Path or URL to the module configuration file. Default is `Modules/modules.yaml`.

- `--sources`, `-s`  
  指定源索引（sources.yaml）路径，默认为 `Modules/sources.yaml`。  
  Path to sources.yaml for module indexes. Default is `Modules/sources.yaml`.

- `--directory`, `-d`  
  指定模块仓库下载目录，默认为 `Modules/`。  
  Output directory for module repositories. Default is `Modules/`.

#### 📤 输出结果 / Output

- 如果配置文件不存在，将自动生成模板
  If the configuration file does not exist, a template will be generated.
- 若模块目录不存在，执行 `git clone --recurse-submodules`  
  If the module directory does not exist, `git clone --recurse-submodules` is executed.
- 若模块已存在，进入目录后执行 `git fetch --all` 和 `git pull`，并根据配置切换分支（如果提供 `@version` 字段）  
  If the module exists, enter the directory and execute `git fetch --all` and `git pull`, and switch branches if a `@version` field is provided.
- 自动递归解析依赖关系，确保依赖模块也被拉取且版本一致  
  Recursively resolves dependencies to ensure all required modules are cloned and version-consistent.
- 每个模块最终存储在 `<output_directory>/<module_name>` 路径中  
  Each module is stored at `<output_directory>/<module_name>`.

输出示例 / Output Example：

```bash
[INFO] Cloning new module: BlinkLED
[EXEC] git clone --recurse-submodules --branch main https://github.com/xrobot-org/BlinkLED Modules/BlinkLED

[SUCCESS] All modules and their dependencies processed.
```

---

### `xrobot_gen_main`

该工具用于根据模块清单和构造参数配置文件，自动生成 C++ 入口函数 `XRobotMain()`，支持嵌套参数、重复实例、多模块构造。
This tool generates a C++ entry function `XRobotMain()` based on the module list and configuration file, supporting nested args, multiple instances, and module composition.

#### 🚀 使用方法 / Usage

```bash
# 自动发现所有模块并生成主函数
# Auto-discover modules and generate main entry
xrobot_gen_main --output User/xrobot_main.hpp

# 指定模块和构造参数配置
# Specify modules and config manually
xrobot_gen_main -o main.cpp -m BlinkLED Motor IMU --config User/xrobot.yaml
```

#### 🎛️ 命令行参数 / Command-Line Arguments

- `--output`, `-o`
  生成的 C++ 文件路径，默认为 `User/xrobot_main.hpp`。
  Output path of generated C++ file, default is `User/xrobot_main.hpp`.

- `--modules`, `-m`
  可选，指定模块名列表；若未指定，则自动扫描 `Modules/` 目录下的模块。
  Optional. List of module names to include. If omitted, auto-discovered from `Modules/`.

- `--hw`
  可选，指定硬件容器变量名，默认为 `hw`。
  Optional. Hardware container variable name. Default: `hw`.

- `--config`, `-c`
  可选，指定构造参数 YAML 文件；若文件不存在，则自动生成。
  Optional. Path to constructor configuration file. Will be created if not found.

#### 📤 输出结果 / Output

- 若未指定 `--config`，将扫描模块头文件中的 `/* === MODULE MANIFEST === */` 区块并生成默认 YAML
  If `--config` is not specified, will scan for `/* === MODULE MANIFEST === */` block and generate default YAML.
- 生成一个包含 `XRobotMain()` 函数的 `.hpp` 或 `.cpp` 文件
  Generates a `.hpp` or `.cpp` file containing the `XRobotMain()` function.
- 每个模块按 `static ModuleName<HardwareContainer> name(hw, appmgr, ...);` 格式实例化
  Each module is instantiated as `static ModuleName<HardwareContainer> name(hw, appmgr, ...);`
- 支持自动添加头文件
  Support for automatically adding header files
- 主循环使用 `appmgr.MonitorAll()` 与 `Thread::Sleep()`
  Main loop uses `appmgr.MonitorAll()` and `Thread::Sleep()`

输出代码示例 / Output Code Example：

```cpp
#include "app_framework.hpp"
#include "libxr.hpp"

// Module headers
#include "BlinkLED.hpp"
#include "BMI088.hpp"
#include "MadgwickAHRS.hpp"

template <typename HardwareContainer>
static void XRobotMain(HardwareContainer &hw) {
  using namespace LibXR;
  ApplicationManager appmgr;

  // Auto-generated module instantiations
  static BlinkLED<HardwareContainer> blinkled(hw, appmgr, 250);
  static BMI088<HardwareContainer> bmi088(hw, appmgr, {1.0, 0.0, 0.0, 0.0}, {0.15, 1.0, 0.1, 0.0, 0.3, 1.0, false}, "bmi088_gyro", "bmi088_accl", 45, 512);
  static MadgwickAHRS<HardwareContainer> madgwickahrs(hw, appmgr, 0.033, "bmi088_gyro", "bmi088_accl", "ahrs_quaternion", "ahrs_euler", 512);

  while (true) {
    appmgr.MonitorAll();
    Thread::Sleep(1000);
  }
}
```

---

### `xrobot_create_mod`

该工具用于快速创建一个符合 XRobot 模块规范的目录结构，包含头文件、README、CMake 配置等，便于模块开发初始化。  
This tool quickly scaffolds a new XRobot module with standard files including header, README, and CMake setup, to accelerate module development.

#### 🚀 使用方法 / Usage

```bash
# 创建一个名为 MyModule 的模块
# Create a module named MyModule
xrobot_create_mod MyModule --desc "LED blinker" --hw led button
```

#### 🎛️ 命令行参数 / Command-Line Arguments

- `class_name`  
  **必填**，模块名称，必须与目录名一致。  
  **Required**. Module name, must match folder name.

- `--desc`  
  模块说明文字。默认值："No description provided"  
  Description text for the module. Default: "No description provided".

- `--hw`  
  所需硬件逻辑接口名列表。默认空。  
  List of required logical hardware interfaces. Default: empty.

- `--constructor`  
  构造参数列表，格式如 `id=foo gpio=LED`。可选。  
  List of constructor args, e.g., `id=foo gpio=LED`. Optional.

- `--template`  
  模板参数列表，格式如 `T=int U=double`。可选。  
  List of template args, e.g., `T=int U=double`. Optional.

- `--depends`  
  依赖模块名列表。可选。  
  List of dependent modules. Optional.

- `--out`  
  输出路径，默认为 `Modules/`。  
  Output folder. Default: `Modules/`.

#### 📂 生成结构 / Generated Structure

假设模块名为 `BlinkLED`，会生成以下文件结构：  
If the module name is `BlinkLED`, the following structure will be generated:

```text
Modules/
└── BlinkLED/
    ├── .github/workflows/build.yml # CI 配置 / CI configuration
    ├── BlinkLED.hpp        # 带 manifest 的头文件 / Header file with manifest
    ├── README.md           # 模块文档 / Module documentation
    └── CMakeLists.txt      # 构建配置 / Build configuration
```

## 💡 CI 用途说明 / What is CI for?

生成的模块会包含一个**GitHub Actions**持续集成（CI）配置：  
- 自动拉取 libxr 依赖并编译模块  
- 支持每次 push、PR、以及每月定时自动测试
- CI 失败能及时发现语法或集成问题  
- 保证你的模块长期可编译、易于合作开发

The generated module includes a GitHub Actions CI workflow that:  
- Pulls libxr as dependency and builds your module  
- Runs on every push, pull request, and a monthly schedule
- Helps catch integration/compile issues automatically  
- Keeps your module always buildable for you and your team

---

### `xrobot_mod_parser`

该工具用于解析 XRobot 模块头文件中的 `MODULE MANIFEST`，提取模块的描述、构造参数、所需硬件等信息，并格式化输出。
This tool parses the `MODULE MANIFEST` block in XRobot module header files, extracting the module description, constructor arguments, required hardware, and formatting the output.

#### 🚀 使用方法 / Usage

```bash
# 解析指定路径下的模块目录或模块集合
# Parse module directory or module collection at the specified path
xrobot_mod_parser --path Modules/BlinkLED
```

```bash
# 解析指定路径下的模块目录或模块集合
# Parse module directory or module collection at the specified path
xrobot_mod_parser --path ./Modules
```

#### 🎛️ 命令行参数 / Command-Line Arguments

- `--path`, `-p`
  **必填**，模块目录路径或模块集合路径。
  **Required**. Path to the module directory or module collection.

#### 📤 输出结果 / Output

- 输出模块描述、构造参数和所需硬件信息。
  Prints the module description, constructor arguments, and required hardware.
  
- 如果模块头文件中的 `constructor_args` 是字典或列表形式，它会被正确解析并格式化为易读格式。
  If `constructor_args` in the module header is in dictionary or list format, it is parsed and formatted into a readable structure.

输出示例 / Output Example：

```bash
=== Module: BlinkLED ===
Description       : 控制 LED 闪烁的简单模块 / A simple module to control LED blinking
Repository        : https://github.com/xrobot-org/BlinkLED

Constructor Args  :
  - blink_cycle        = 250

Required Hardware : led/LED/led1/LED1
```

---

### `xrobot_setup`

该工具用于自动配置 XRobot 环境，生成模块仓库配置以及主函数代码。它通过调用 `xrobot_init_mod` 初始化模块，并根据构造参数自动生成主函数代码。
This tool automates the XRobot setup, generating module repository configuration and main function code. It initializes modules via `xrobot_init_mod`, and automatically generates the main function code based on constructor parameters.

#### 🚀 使用方法 / Usage

```bash
# 自动配置并生成主函数代码
# Auto-configure and generate main function code
xrobot_setup
```

然后在`add_subdirectory(your_libxr_path/LibXR)`之前，添加如下内容：  
Then add the following before `add_subdirectory(your_libxr_path/LibXR)`:

```cmake
set(XROBOT_MODULES_DIR YOUR_MODULES_PATH) # eg. ${CMAKE_CURRENT_SOURCE_DIR}/Modules
```

#### 🎛️ 命令行参数 / Command-Line Arguments

- `--config`, `-c`
  可选，指定构造参数配置文件路径，可以是本地文件或远程 URL。
  Optional. Path to constructor configuration file, can be a local file or a URL.

#### 📤 输出结果 / Output

- **默认配置文件**：
  如果 `Modules/modules.yaml` 不存在，脚本会生成一个默认配置文件。
  If `Modules/modules.yaml` doesn't exist, a default configuration file will be created.
  
- **模块初始化**：
  调用 `xrobot_init_mod` 命令初始化模块仓库，拉取模块代码。
  Calls `xrobot_init_mod` to initialize the module repository and clone modules.

- **主函数生成**：
  生成 `User/xrobot_main.hpp`，包含自动生成的 `XRobotMain()` 函数代码。
  Generates `User/xrobot_main.hpp` with the auto-generated `XRobotMain()` function code.

输出示例 / Output Example：

```bash
Starting XRobot auto-configuration

[INFO] Default config created: ./Modules/modules.yaml
Please edit the file and rerun.

[EXEC] xrobot_init_mod --config ./Modules/modules.yaml -d ./Modules
[SUCCESS] All modules and their dependencies processed.

[EXEC] xrobot_gen_main --output ./User/xrobot_main.hpp --config ./User/xrobot.yaml
[INFO] Constructor config generated: ./User/xrobot.yaml
Modify the configuration and rerun to apply changes

[EXEC] xrobot_gen_main --output ./User/xrobot_main.hpp --config ./User/xrobot.yaml
All done! Main function generated at: ./User/xrobot_main.hpp
```

---

### `xrobot_src_man`

该工具用于管理和聚合多个模块源（索引），支持模块仓库的查询、源的增删、模板生成与多源环境下的模块查找。  
This tool manages and aggregates multiple module sources (indexes), supports module repo querying, adding/removing sources, generating source/index templates, and searching modules across mirrors.

#### 🚀 使用方法 / Usage

```bash
# 列出所有可用模块
xrobot_src_man list

# 查询某模块的仓库地址与源
xrobot_src_man get xrobot-org/BlinkLED

# 查找所有包含某模块的源（支持多源和镜像）
xrobot_src_man find xrobot-org/BlinkLED

# 生成 sources.yaml 模板
xrobot_src_man create-sources --output my_sources.yaml

# 添加一个 index.yaml 源到 sources.yaml
xrobot_src_man add-source https://mydomain.com/index.yaml --priority 1 --sources Modules/sources.yaml

# 生成 index.yaml 模板
xrobot_src_man create-index --output Modules/index.yaml --namespace myns --mirror-of xrobot-org

# 向 index.yaml 添加模块仓库地址
xrobot_src_man add-index https://github.com/yourorg/MyModule.git --index Modules/index.yaml
```

#### 🎛️ 子命令说明 / Subcommands

- `list`  
  列出所有唯一模块 ID（namespace/ModuleName）及其来源。  
  List all unique modules (namespace/ModuleName) and their source.

- `get <modid>`  
  查询指定模块的仓库地址与源信息。  
  Get repository URL and source info for the given module.

- `find <modid>`  
  查询所有包含该模块的源（支持多源镜像查找）。  
  Find all sources containing the module (multi-mirror support).

- `create-sources [--output]`  
  生成 sources.yaml 模板，支持自定义路径。  
  Generate a template sources.yaml file (custom path supported).

- `add-source <url> [--priority] [--sources]`  
  向 sources.yaml 添加一个新的 index.yaml 源，支持优先级。  
  Add an index.yaml source to sources.yaml with priority.

- `create-index [--output] [--namespace] [--mirror-of]`  
  生成模块 index.yaml 模板，支持自定义命名空间与镜像配置。  
  Generate index.yaml template with custom namespace/mirror-of.

- `add-index <repo_url> --index <index.yaml>`  
  向 index.yaml 添加模块仓库地址。  
  Add a module repository URL to the specified index.yaml.

#### 📤 输出结果 / Output

- 自动聚合并显示所有模块及其源（支持镜像、优先级与多源管理）。  
  Aggregates and displays all modules and their sources (mirrors, priority, multi-source supported).
- 支持标准 yaml 索引文件的模板快速生成与批量维护。  
  Supports quick generation and batch maintenance of standard yaml index files.

输出示例 / Output Example：

```bash
Available modules:
  xrobot-org/BlinkLED               source: https://xrobot-org.github.io/xrobot-modules/index.yaml (actual namespace: xrobot-org)
  yourns/MyModule                   source: https://mydomain.com/index.yaml (mirror of: xrobot-org) (actual namespace: yourns)
```

---

## 🧪 快速上手 / Quickstart

```bash
# 1. 一键初始化环境、拉取模块、生成主函数（推荐）
# 1. One-click initialize workspace, fetch modules, and generate main function (recommended)
$ xrobot_setup
Starting XRobot auto-configuration...
[INFO] Created default Modules\modules.yaml
Please edit this file; each line should be a full module name like:
  - xrobot-org/BlinkLED
  - your-namespace/YourModule@dev
[INFO] Created default Modules\sources.yaml
Please configure sources index.yaml for official or custom/private mirrors.
Default official source already included.

$ xrobot_setup
Starting XRobot auto-configuration...
[EXEC] xrobot_init_mod --config Modules\modules.yaml --directory Modules --sources Modules\sources.yaml
[INFO] Cloning new module: xrobot-org/BlinkLED
Cloning into 'Modules\BlinkLED'...
remote: Enumerating objects: 31, done.
remote: Counting objects: 100% (31/31), done.
remote: Compressing objects: 100% (21/21), done.
remote: Total 31 (delta 10), reused 31 (delta 10), pack-reused 0 (from 0)
Receiving objects: 100% (31/31), 4.43 KiB | 2.22 MiB/s, done.
Resolving deltas: 100% (10/10), done.
[SUCCESS] All modules and their dependencies processed.
[EXEC] xrobot_gen_main --output User\xrobot_main.hpp
Discovered modules: BlinkLED
[INFO] Successfully parsed manifest for BlinkLED
[INFO] Writing configuration to User\xrobot.yaml
[SUCCESS] Generated entry file: User\xrobot_main.hpp

All done! Main function generated at: User\xrobot_main.hpp

# 2. 单独拉取/同步模块仓库（可选）
# 2. Pull or sync module repositories separately (optional)
$ xrobot_init_mod --config Modules/modules.yaml --directory Modules
[INFO] Updating module: xrobot-org/BlinkLED
Already up to date.
Already on 'master'
Your branch is up to date with 'origin/master'.
[SUCCESS] All modules and their dependencies processed.

# 3. 创建模块
# 3. Create a module
$ xrobot_create_mod MySensor --desc "IMU interface module" --hw i2c1
[OK] Module MySensor generated at Modules\MySensor

# 4. 查看模块信息
# 4. View module information
$ xrobot_mod_parser --path ./Modules/MySensor/

=== Module: MySensor.hpp ===
Description       : IMU interface module

Constructor Args  :
Required Hardware : i2c1
Depends           : None

# 5. 添加模块仓库（如需自定义来源）
# 5. Add a module repository (custom source, optional)
$ xrobot_add_mod your-namespace/YourModule@main
[SUCCESS] Added repo module 'your-namespace/YourModule@main' to Modules\modules.yaml

# 6. 添加模块实例
# 6. Add a module instance
$ xrobot_add_mod MySensor
[SUCCESS] Appended module instance 'MySensor' as id 'MySensor_0' to User\xrobot.yaml

# 7. 查看模块实例
# 7. View module instances
$ cat ./User/xrobot.yaml
global_settings:
  monitor_sleep_ms: 1000
modules:
- name: BlinkLED
  constructor_args:
    blink_cycle: 250
- id: MySensor_0
  name: MySensor
  constructor_args: {}

# 8. 重新生成主函数（支持自动扫描和配置）
# 8. Regenerate main function (auto scan & config supported)
$ xrobot_gen_main --output User/xrobot_main.hpp
Discovered modules: BlinkLED, MySensor
[SUCCESS] Generated entry file: User/xrobot_main.hpp

# 9. 查看主函数
# 9. View main function
$ cat ./User/xrobot_main.hpp
#include "app_framework.hpp"
#include "libxr.hpp"

// Module headers
#include "BlinkLED.hpp"
#include "MySensor.hpp"

static void XRobotMain(LibXR::HardwareContainer &hw) {
  using namespace LibXR;
  ApplicationManager appmgr;

  // Auto-generated module instantiations
  static BlinkLED blinkled(hw, appmgr, 250);
  static MySensor MySensor_0(hw, appmgr);

  while (true) {
    appmgr.MonitorAll();
    Thread::Sleep(1000);
  }
}
```

---

## 📖 更多信息 / More Information

- [GitHub Repository](https://github.com/xrobot-org/XRobot)
- [Documentation](https://xrobot-org.github.io)
- [Issue Tracker](https://github.com/xrobot-org/XRobot/issues)
