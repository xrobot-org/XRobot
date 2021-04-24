# 如何贡献代码

感谢你阅读此文档，现阶段代码主要由战队队员修改，但是欢迎大家提出意见和建议。此文档主要为指导队员如何提交自己的代码。

## 测试

现阶段并没有软件测试，所有功能必须在机器人或MCU上测试之后提交。

## 提交代码更改

请在仓库(Repo)中提交*Pull Request*(*PR*)来提交你的修改。*PR*必须要有清楚的描述。

*Commit*的日志必须要清楚地描述改动。小改动允许日志只有一行，但是大的改动最好有多行日志来详细描述。推荐使用*VS Code*内置的工具来操作*Git*。

## 风格指南

阅读代码和README来了解整个代码的结构。为了更好的可读性：

* **必须**先阅读[风格指南](Doc/style_guide.md)并严格遵守

## 添加新的CLI命令

添加到CLI的命令可以说是调试时候使用的，也可以是运行时使用的，也可以是教学时帮助理解的实验。具体可以参考[添加判断字节顺序的实验](https://github.com/qsheeeeen/qdu-robomaster-mcu/commit/150460147dbc4f5b0ad7054c2591e01bba452495)和[FreeRTOS CLI官方教程](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_Command_Line_Interface.html)。

## 添加新的开发板

需要修改的文件夹有`bsp`。

1. 在bsp中与`a`、`c`并列添加一个文件夹。例如`b`（代指RM开发板B型）。
1. 参考`bsp`中头文件，在`b`中添加对应的源代码文件。
    * 在头文件中不能实现的功能，需要加宏定义来根据不同的开发板来编译。例如[led.h](./User/bsp/led.h)中针对不同板子定义了不同的LED功能。

## 添加新的组件

需要修改的文件夹有`component`。

步骤：

1. 在`component`新建一对文件`xx.c`和`xx.h`。

## 添加新的设备（传感器、执行器等）

需要修改的文件夹有`device`。

步骤：

1. 在`device`新建一对文件`xx.c`和`xx.h`。

注意事项：

* Device里的设备只能在thread里使用，因为数据的读取需要用到任务同步机制。

## 添加新的机构

机构是指不同于底盘、发射器、云台的部分机器人，例如工程机器人的执行机构。

需要修改的文件夹有`task`和`module`，这里假设已经完成所需要的传感器和执行器的添加。

## 添加任务

需要修改的文件夹有`task`。

1. 在`task`新建文件`xx.c`。
1. 在`task\user_task.h`中添加任务函数的声明`Task_Xxxx`。
1. 在`task\user_task.h`中添加任务属性的声明`attr_xxx`。
1. 在`task\user_task.c`中添加任务属性的定义`attr_xxx`。

注意事项：

* `osKernelLock`和`osKernelUnlock`中间不能you可能导致任务切换的函数，例如`osMessageQueuePut`。
* 如果任务不需要运行多个实例，则尽量将任务变量放置到任务函数外。

---

万分感谢

青岛大学 未来战队
