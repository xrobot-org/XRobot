# XRobot

一套可在PC/MCU/模拟器运行的机器人控制系统，诞生于Robomaster比赛，但绝不局限于此。

![平衡](./doc/image/平衡.gif "平衡") ![英雄](./doc/image/英雄.gif "英雄") ![工程](./doc/image/工程.gif "工程") ![飞镖架](./doc/image/飞镖架.gif "飞镖架")

## 硬件支持

| MCU           | Board                                                                                                                                                              | Image                                                              |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------------ |
| STM32F407IG   | [Robomaster C型开发板](https://www.robomaster.com/zh-CN/products/components/general/development-board-type-c/info)                                                 | <img src="doc/image/rm-c.png" width="500" height="220">            |
| STM32F302CB   | [IMU-DEV-BOARD](https://xrobot-org.github.io/1.hardware/3.IMU-DEV.html)                                                                                            | <img src="doc/image/IMU-DEV-1.jpg" width="350" height="200">       |
| STM32F446RE   | [C-Mini](https://xrobot-org.github.io/1.hardware/1.C-MINI.html)                                                                                                    | <img src="doc/image/C-MINI-1.jpg" width="350" height="250">        |
| STM32F103     | Wait for update                                                                                                                                                    | Wait for update                                                    |
| ESP32-C3      | [ESP32C3-Core](https://xrobot-org.github.io/1.hardware/2.ESP32C3-Core.html)                                                                                        | <img src="doc/image/esp32c3-core-1.jpeg" width="260" height="200"> |
| Linux x86/arm | [Intel NUC](https://www.intel.cn/content/www/cn/zh/products/details/nuc.html)/[Jetson TX2](https://www.nvidia.cn/autonomous-machines/embedded-systems/jetson-tx2/) | <img src="doc/image/nuc.jpg" width="350" height="200">             |

## 主要特色

* 利用CMake & Clang实现跨平台开发
* VSCode一键编译调试
* ClangTidy代码检查
* 完善的[文档](https://xrobot-org.github.io)
* BSP兼容层使用纯C实现，上层代码使用C++，稳定实时，开发方便
* 一个项目适配不同型号的机器人型号，现已支持步兵/英雄/哨兵/平衡
* 兼容多开发板和操作系统，支持Linux原生运行和Linux上的Webots仿真
* 兵种配置文件及图形化配置
* 操作手自定义UI
* 命令行界面（CLI）
* USB上位机控制

## 图片展示

利用命令行可以辅助调试程序、校准开发板、初始化机器人、读取不同参数配置。

|

![VSCode调试界面](./doc/image/调试界面.png?raw=true "VSCode调试界面")

 |                  |
 | :--------------: |
 | *VSCode调试界面* |

|

![Webots仿真](./doc/image/Webots仿真.png?raw=true "Webots仿真")

 |              |
 | :----------: |
 | *Webots仿真* |

|

![命令行](./doc/image/命令行.png?raw=true "命令行")

 |          |
 | :------: |
 | *命令行* |

|

![客户端UI](./doc/image/客户端UI.png?raw=true "客户端UI")

 |            |
 | :--------: |
 | *客户端UI* |

## 使用和入门

请阅读我们的[文档](https://xrobot-org.github.io)
