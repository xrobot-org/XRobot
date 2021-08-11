# Style Guide

编码风格主要参考Google C++ 风格指南([中文版](https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/contents/)/[英文原版](https://google.github.io/styleguide/cppguide.html))中与C语言相通的部分，并在此基础上添加了部分战队特有风格。

本文件为对以上材料的补充和改写。如果本文件与参考指南冲突，则以本指南为准。

所有不符合此规范的代码都不能提交，鼓励对已有的不规范的代码进行更改。

* 提交前**必须**使用Google风格的`clang-formator`进行格式化
* 如果使用VS Code编辑代码，建议打开`Format On Save`

## 头文件

### #pragma once 保护

所有头文件都应该使用 `#pragma once` 来防止头文件被多重包含。

### #include的路径和顺序

所有手写的代码都要放在`User`内，以保证不需要添加其他`include`路径。

1. 本源代码文件对应的头文件。本文件为`adc.c`则对应 `adc.h`。

1. C自带库，需要使用`#include <>`。

1. 其他文件夹内或本文件夹内的 `.h` 文件，使用`#include ""`，按字母数序排列，应包含路径。

## 面向过程编程

所有的代码均运用面向过程的编程思维，用`struct`存储数据，使用函数对`struct`内的数据进行操作。

## 函数

### 参数

1. 尽量使用指针，来保证效率。
1. 单一数据输出时使用`return`的形式，多输出时使用结构体指针进行输出。
1. 函数的参数应该尽可能地用`const`保护起来，防止意外修改。
1. 对于指针参数，应该检查参数是否为`NULL`。

## 命名约定

1. 为保证命名不重复，需要以所在文件名夹为函数名称的开始
    * 例如`bsp/buzzer.h`内，宏定义命名需要以`BSP_BUZZER_`开始，函数则以`BSP_Buzzer_`开始。
    * 前缀之后的部分，遵顼Google C++ 风格指南。例如`BSP_GPIO_RegisterCallback()`

1. `typedef`定义的类后面加`_t`，例：`PID_t`

## 注释

使用`/* 内容 */`格式的注释(注意空格)

## 格式

待补充

## 其他

``` C
/* 当有代码不需要起作用但要保留时，请使用以下方法 */
#if 0

FunctionOne();
FunctionTwo();

#endif
```
