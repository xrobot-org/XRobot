# 青岛大学 RoboMaster 嵌入式 代码开源

***Developing***

***稳健开发测试中***

## 软件功能介绍

本开源软件为青岛大学未来战队机器人的嵌入式控制代码。参考了官方开源和其他战队代码，结合对其他嵌入式项目的理解（如PX4），从零编写而成。中心思想：

- 利用好RTOS和中断，释放CPU性能，保证实时性。
- 一个项目适配不同型号的机器人和不同的操作手。

这样做增加代码了的重用，减少了工作量。实现了可以通过命令行，切换所适配的机器人和操作手。代码完成后只需要烧写一次，减少维护的工作量，减少出错的可能性。

## 图片展示

| ![命令行界面（CLI）](./Image/命令行界面.png?raw=true "命令行界面（CLI）") |
|:--:|
| *命令行界面（CLI）* |

| ![客户端UI](./Image/客户端UI.png?raw=true "客户端UI") |
|:--:|
| *客户端UI* |

利用命令行可以辅助调试程序、校准开发板、初始化机器人、读取不同参数配置。

## 依赖&环境

- Windows平台下用CubeMX生成项目，然后用Keil uvesrion进行编辑、烧写和调试。

- TODO：利用CMake和VS Code实现跨平台开发，减少对收费软件和操作系统的依赖。

## 使用说明

- 环境安装
  - [MDK-ARM](https://www.keil.com/) （必备）
  - [STM32CubeMX](https://www.st.com/zh/development-tools/stm32cubemx.html) （可选）

- 克隆本库 `git clone --recursive https://gitee.com/qsheeeeen/qdu-rm-mcu.git`

  - 因为本软件中与视觉通信部分用的是与视觉共同维护的一个库，因此要加上`--recursive`，或在克隆完成后执行

    `git submodule init && git submodule update`

- （可选）针对不同板子需要到不同的CubeMX工程文件（DevA.ioc、DevC.ioc）。

  - 利用CubeMX生成对应的外设初始化代码和Keil工程文件。

  - 每次生成代码后，请利用Git丢弃Middlewares文件夹中的所有改变。原因如下。

    1. 使用了AC6编译器，与CubeMX默认不匹配，会影响到FreeRTOS的移植。

  - 因为已经生成过Keil工程文件，所以只会覆盖以前生成的代码，而不会影响手写的代码。

- 打开MDK-ARM中的DevC.uvprojx（或DevA.uvprojx）即可进行编辑、烧写或调试。

- Keil工程中有两个Target，其中Debug用来调试，不包含编译器优化等；DevC/DevA用来编译输出最终固件。

## 文件目录结构&文件用途说明

| 文件夹 | 来源 | 内容 |
| ---- | ---- | ----  |
| Core | CubeMX | 包含核心代码，外设初始化，系统初始化等 |
| Doc | 开发者 | 文档 |
| Drivers | CubeMX | CMSIS相关库、STM32 HAL |
| Image | 开发者 | 图片 |
| MDK-ARM | CubeMX | Keil uversion 项目相关文件 |
| Middlewares | 开发者 / CubeMX | 中间件 |
| USB_DEVICE | CubeMX | USB相关文件 |
| User |  开发者 | 手动编写的代码 |
| Utils |  开发者 | 使用到的工具，如CubeMonitor, Matlab |

| User内 | 内容 |
| ---- | ----  |
| bsp | 文件夹内包含开发板信息，基于STM32 HAL对板载的外设进行控制，方便适配各种开发板 |
| component | 包含各种组件，自成一体，相互依赖，但不依赖于其他文件夹 |
| device | 独立于开发板的设备，依赖于HAL和bsp |
| module | 对机器人各模块的抽象，各模块一起组成机器人 |
| task | 独立的任务，module的运行容器，也包含通信、姿态解算等 |

## 系统介绍

| ![硬件系统框图（全官方设备）](./Image/步兵嵌入式硬件框图.png?raw=true "硬件系统框图（全官方设备）") |
|:--:|
| *硬件系统框图（全官方设备）* |

| ![软件流程图](./Image/嵌入式程序流程图.png?raw=true "软件流程图") |
|:--:|
| *软件流程图* |

| ![嵌入式程序结构图](./Image/嵌入式程序结构图.png?raw=true "嵌入式程序结构图") |
|:--:|
| *嵌入式程序结构图* |

## 原理介绍

### 云台控制原理

| ![云台控制原理（与PX类似）](./Image/云台控制原理.png?raw=true "嵌入式程序结构图") |
|:--:|
| *云台控制原理（与PX类似）* |

### 其他参考文献

- 软件架构参考[PX4 Architectural Overview](https://dev.px4.io/master/en/concept/architecture.html)

- 云台控制参考[PX4 Controller Diagrams](https://dev.px4.io/master/en/flight_stack/controller_diagrams.html)

- 底盘Mixer和CAN的Control Group参考[PX4 Mixing and Actuators](https://dev.px4.io/master/en/concept/mixing.html)

- PID请参考[pid.c](User\component\pid.c)

## TODO

- 解决`-O3`引起的BUG

## Roadmap

1. 添加更全面的注释

1. 完善与上位机通信，与视觉组配合完成自瞄、打击能量机关和哨兵全自主运行

1. 添加上手指南
