# 青岛大学 RoboMaster 嵌入式 代码开源

***Developing***

***开发测试中***

[Gitee](https://gitee.com/qsheeeeen/qdu-rm-mcu)
[Github](https://github.com/qsheeeeen/qdu-rm-mcu)

## 软件功能介绍

本开源软件为青岛大学未来战队机器人的嵌入式控制代码。参考了官方开源和其他战队代码，结合对其他嵌入式项目的理解（如PX4），从零编写而成。

主要特色：

- 实时性强，CPU利用率低
- 一个项目适配不同型号的机器人和不同的操作手。
- 利用Cmake & GCC实现跨平台开发
- 不依赖特定操作系统和IDE

实现了可以通过命令行，切换所适配的机器人和操作手。代码完成后只需要烧写一次，减少维护的工作量，减少出错的可能性。用到的系统和软件全部免费。

## 图片展示

利用命令行可以辅助调试程序、校准开发板、初始化机器人、读取不同参数配置。

| ![命令行界面（CLI）](./image/命令行界面.png?raw=true "命令行界面（CLI）") |
|:--:|
| *命令行界面（CLI）* |

| ![客户端UI](./image/客户端UI.png?raw=true "客户端UI") |
|:--:|
| *客户端UI* |

## 推荐开发配置

### 系统

- Windows Subsystem for Linux (WSL) 2 Ubuntu [安装说明](https://docs.microsoft.com/zh-cn/windows/wsl/install-win10)

- Ubuntu native [详细信息](https://ubuntu.com)

### 配置环境

- [Visual Studio Code](https://code.visualstudio.com/)
  - 安装必备插件`C/C++` `CMake`
- 安装构建工具`sudo apt install cmake gcc-arm-none-eabi ninja-build`

### 获取源代码

1. 克隆本库 `git clone --recursive https://gitee.com/qsheeeeen/qdu-rm-mcu.git`

或者

1. `git clone https://gitee.com/qsheeeeen/qdu-rm-mcu.git`
1. `git submodule init && git submodule update`

### 编译固件

- 命令行操作
  1. `cmake -DCMAKE_TOOLCHAIN_FILE:STRING=cmake/arm-none-eabi.cmake -H. -B./build -G Ninja`
  1. `cd build && ninja`

- VS Code 操作
![命令行界面（CLI）](./image/VSCode编译固件.png?raw=true "命令行界面（CLI）")
  1. 选择构建类型
  1. 编译

### 调试 & 烧写

#### Ozone

- Ubuntu native
  1. 安装Jlink驱动
  1. 安装Ozone Linux版
  1. 正常调试

- Windows 11 WSL with WSLg
  1. Windows Host中安装Jlink驱动
  1. WSL中安装Ozone Linux版
  1. WSL中使用Ozone调试，通过网络连接Jlink

#### OpenOCD

TODO

## 文件目录结构&文件用途说明

| 文件夹 | 来源 | 内容 |
| ---- | ---- | ----  |
| build | CMake | 构建产物 |
| cmake | 开发者 | cmake脚本 |
| doc | 开发者 | 文档 |
| image | 开发者 | 图片 |
| ld | 开发者 | 链接脚本 |
| src | 开发者 | 源代码 |
| third_party | 开发者 | 第三方仓库 |
| utils |  开发者 | 使用到的工具，如CubeMonitor, Matlab |

| src | 内容 |
| ---- | ----  |
| bsp | 文件夹内包含开发板信息，基于STM32 HAL对板载的外设进行控制，方便适配各种开发板 |
| component | 包含各种组件，自成一体，相互依赖，但不依赖于其他文件夹 |
| device | 独立于开发板的设备，依赖于HAL和bsp |
| firmware | 固件入口 |
| hal | 硬件抽象层 |
| middleware | 中间件，基于RTOS为thread提供功能 |
| module | 对机器人各模块的抽象，各模块一起组成机器人 |
| rtos | 实时操作系统 |
| thread | 独立的线程，module的运行容器，也包含通信、姿态解算等 |
| usb | USB库 |

## 系统介绍

| ![硬件系统框图（全官方设备）](./image/步兵嵌入式硬件框图.png?raw=true "硬件系统框图（全官方设备）") |
|:--:|
| *硬件系统框图（全官方设备）* |

| ![嵌入式程序数据流向图](./image/嵌入式程序数据流向图.png?raw=true "嵌入式程序数据流向图") |
|:--:|
| *嵌入式程序数据流向图* |

| ![嵌入式程序层次图](./image/嵌入式程序层次图.png?raw=true "嵌入式程序层次图") |
|:--:|
| *嵌入式程序层次图* |

## 原理介绍

### 云台控制原理

| ![云台控制原理（与PX类似）](./image/云台控制原理.png?raw=true "嵌入式程序层次图") |
|:--:|
| *云台控制原理（与PX类似）* |

### 其他参考文献

- 软件架构参考[PX4 Architectural Overview](https://dev.px4.io/master/en/concept/architecture.html)

- 云台控制参考[PX4 Controller Diagrams](https://dev.px4.io/master/en/flight_stack/controller_diagrams.html)

- 底盘Mixer和CAN的Control Group参考[PX4 Mixing and Actuators](https://dev.px4.io/master/en/concept/mixing.html)

- PID请参考[pid.c](src/component/comp_pid.c)

### TODO

- [x] 利用好CCM RAM[参考文档](https://www.st.com/resource/en/application_note/an4296-use-stm32f4stm32g4-ccm-sram-with-iar-embedded-workbench-keil-mdkarm-stmicroelectronics-stm32cubeide-and-other-gnubased-toolchains-stmicroelectronics.pdf)
- [ ] 使用[TinyUSB](https://github.com/hathach/tinyusb)
