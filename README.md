# 青岛大学 RoboMaster 嵌入式 代码开源

***UNFINISHED & UNTESTED.***

大部分代码为疫情期间在开发板C型上开发，未连接任何设备。项目编译通过，无内存泄漏，测试了部分功能，但大部分逻辑因为没有条件而没测试。

## 软件功能介绍

本开源软件为青岛大学未来战队机器人的嵌入式控制代码。参考了官方开源和其他战队代码，结合对其他嵌入式项目的理解（如PX4），从零编写而成。中心思想：

- 利用好RTOS和中断，释放CPU性能，保证实时性。
- 一个项目适配不同不通型号的机器人和不同的操作手。

这样做增加代码了的重用，减少了工作量。实现了可以通过命令行，切换所适配的机器人和操作手。代码完成后只需要烧写一次，减少维护的工作量，减少出错的可能性。

## 图片展示

### 命令行界面（CLI）

![命令行界面（CLI）](./Image/命令行界面.png "命令行界面（CLI）")

## 依赖&环境

- Windows平台下用CubeMX生成项目，然后用Keil uvesrion进行编辑、烧写和调试。

- TODO：利用CMake和VS Code实现跨平台开发，减少对收费软件和操作系统的依赖。

## 使用说明

- 针对不同板子需要到不同的CubeMX工程文件DevA.ioc、DevC.ioc）。

- 利用CubeMX生成对应的外设初始化代码和Keil工程文件。

  - 所以每次生成代码后，请利用Git丢弃Middlewares文件夹中的所有改变。原因如下。

    1. 使用了AC6，与CubeMX默认不匹配，会影响到FreeRTOS的移植。

    2. 使用了比CubeMX更新的FreeRTOS版本，降版本会导致部分代码无法编译。

  - 因为已经生成过Keil工程文件，所以只会覆盖以前生成的代码，而不会影响手写的代码。

- 打开MDK-ARM中的DevC.uvprojx（或DevA.uvprojx）即可进行编辑、烧写或调试。

- Keil工程中有两个Target，其中Debug用来调试，不包含编译器优化等；DevC/DevA用来编译输出最终固件。

## 文件目录结构及文件用途说明

| 文件夹 | 内容 |
| ---- | ----  |
| Core | CubeMX生成。包含核心代码，外设初始化，系统初始化等 |
| Drivers | CubeMX生成。包含CMSIS相关库和STM32 HAL |
| Image | 图片 |
| MDK-ARM | CubeMX生成。Keil uversion 项目相关文件 |
| Middlewares | 中间件。部分CubeMX生成，部分手动添加 |
| USB_DEVICE | CubeMX生成。USB相关文件 |
| User | 手动编写的代码 |

| User内 | 内容 |
| ---- | ----  |
| bsp | 文件夹内包含开发板信息，基于STM32 HAL对板载的外设进行控制|
| component | 包含各种组件，自成一体，相互依赖，但不依赖于其他文件夹|
| device | 独立于开发板的设备，依赖于HAL和bsp|
| module | 对机器人各模块的抽象，各模块一起组成机器人|
| task | 独立的任务，module的运行容器，也包含通信、姿态解算等 |

## 系统介绍

### 硬件系统框图

![步兵嵌入式硬件框图](./Image/步兵嵌入式硬件框图.png "步兵嵌入式硬件框图")

### 软件流程图

![步兵嵌入式硬件框图](./Image/嵌入式程序流程图.png "步兵嵌入式硬件框图")

### 软件层级图

![嵌入式程序结构图](./Image/嵌入式程序结构图.png "嵌入式程序结构图")

## 原理介绍

### 云台控制原理

![云台控制原理（与PX中相同）](./Image/云台控制原理.png "嵌入式程序结构图")

### 其他参考文献

- 软件架构参考了[PX4 Architectural Overview](https://dev.px4.io/master/en/concept/architecture.html)

- 云台控制可以参考[PX4 Controller Diagrams](https://dev.px4.io/master/en/flight_stack/controller_diagrams.html)

- 底盘Mixer和CAN的Control Group参考了[PX4 Mixing and Actuators](https://dev.px4.io/master/en/concept/mixing.html)

## TODO

- Port LittleFS.
- 添加 logging.
- 添加 gimbal 软限位.
- 给BSP想个更好的名字，备选"mcu"。
- 给BSP USB print加保护，允许不同进程的使用。
  - 给所有BSP加保护
  - device.c里面加上一个Device_Init()，在里面初始化所有mutex
- CAN设备代码优化。消息解析发送方向。
  - CAN设备动态初始化，保存好几组配置。

## Roadmap

近期：

1. 在步兵上完成所有代码的调试工作。
    - 根据培训反馈添加注释

1. 只调整参数，完成对哨兵的适配。

1. 添加与上位机的通信协议，完成联调。

远期：

1. 添加单元测试。

1. 移植PX4多轴控制部分来控制云台

1. 备赛过程根据进度完成对各兵种的适配。
