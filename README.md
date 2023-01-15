# XRobot

一套可在PC/MCU/模拟器运行的机器人控制系统，诞生于Robomaster比赛，但绝不局限于此。

## 主要特色

- 利用CMake & Clang实现跨平台开发
- VSCode一键编译调试
- ClangTidy代码检查
- 完善的[文档](https://xrobot-org.github.io)
- BSP兼容层使用纯C实现，上层代码使用C++，稳定实时，开发方便
- 一个项目适配不同型号的机器人型号，现已支持步兵/英雄/哨兵/平衡
- 兼容多开发板和操作系统，支持Linux原生运行和Linux上的Webots仿真
- 兵种配置文件及图形化配置
- 操作手自定义UI
- 命令行界面（CLI）
- USB上位机控制

## 图片展示

利用命令行可以辅助调试程序、校准开发板、初始化机器人、读取不同参数配置。

| ![VSCode调试界面](./doc/image/调试界面.png?raw=true "VSCode调试界面") |
| :-------------------------------------------------------------------: |
|                           *VSCode调试界面*                            |

| ![Webots仿真](./doc/image/Webots仿真.png?raw=true "Webots仿真") |
| :-------------------------------------------------------------: |
|                          *Webots仿真*                           |

| ![命令行](./doc/image/命令行.png?raw=true "命令行") |
| :-------------------------------------------------: |
|                      *命令行*                       |

| ![客户端UI](./doc/image/客户端UI.png?raw=true "客户端UI") |
| :-------------------------------------------------------: |
|                        *客户端UI*                         |

## 推荐开发配置

### 系统

- Ubuntu native [详细信息](https://ubuntu.com)

### 配置环境

- [Visual Studio Code](https://code.visualstudio.com/)
  - 安装必备插件`C/C++` `CMake`
- 安装构建工具`sudo apt install cmake gcc-arm-none-eabi clang clangd ninja-build python3-tk`

### 获取源代码

1. 克隆本库 `git clone --recursive https://gitee.com/qsheeeeen/qdu-rm-mcu.git`

或者

1. `git clone https://gitee.com/qsheeeeen/qdu-rm-mcu.git`
1. `git submodule init && git submodule update`
