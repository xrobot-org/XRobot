# 如何贡献代码

感谢你阅读此文档，现阶段代码主要由战队队员修改，但是欢迎大家提出意见和建议。此文档主要为指导队员如何提交自己的代码。

## 测试

现阶段并没有软件测试，所有功能必须在机器人或MCU上测试之后提交。

## 提价代码更改

请在仓库（Repo）中提交Pull Request来提交你的修改

Commit的日志必须要有清楚的描述。小改动可以接收一行日志，但是大的改动最好有多行日志的详细描述。

    $ git commit -m "A brief summary of the commit
    > 
    > A paragraph describing what changed and its impact."

## 编码约定

阅读代码和README来了解整个代码的结构。为了更好的可读性：

* 参考Google C++ 代码风格指南中与C语言相通的部分
* 用两个空格代替Tab
* 使用`/* 内容 */`格式的注释
* 提交前需要使用Google风格的clang-formator进行格式化
* 所有命名参考已有程序和Wiki

万分感谢

青岛大学 未来战队
