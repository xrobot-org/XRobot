# CubeMonitor

> ## 简介
>
> > CubeMonitor是ST公司提供的一款组态软件，用于监控修改STMCU的运行、修改变量等。

> ## 快速开始
>
> > 安装CubeMonitor
> >
> > > 请到ST官网下载安装CubeMonitor
> > >
> > > [STM32CubeMonitor - STM32CubeMonitor runtime variable monitoring and visualization tool for STM32 products - STMicroelectronics](https://www.st.com/zh/development-tools/stm32cubemonitor.html)
>
> > 导入文件
> >
> > > * 打开CubeMonitor
> > >
> > > * 将工作区的所有节点删除，并点击右上角设置->导入
> > > * 选择剪切板中的"选择要导入的文件"按钮
> > > * 选择本目录下QDU-RM-MCU-Monitor.json文件并导入
>
> > 捕获变量、绘制图表
> >
> > > * 双击myVariables节点，点击Exexcutable后面的小铅笔，输入Name(任意)，以及Keil生成的axf文件目录，CubeMonitor会自动搜索axf文件，并显示在File一栏
> > > * 在下方的Variable List中选择要监控的变量
> > > * 注意：对于一些特殊的变量，比如数组，CubeMonitor不能选择，因此可以手动添加。双击myVariables节点后，点击Variable List下方的Add custom variable手动添加。
> > > * OutputToFile节点的作用是将捕获到的数据转换成JSON文件，并存储在本地磁盘上。因此，需要双击本节点，对存储的目标文件进行修改。
> >
> > 对运行中的程序中的变量进行修改
> >
> > > * 双击myVariables2节点，将要动态修改的变量添加到此节点，过程同上。
> >
> > 部署运行
> >
> > > 点击右上角DEPLOY后点击DASHBOARD
> >
> > JSON转换为CSV
> >
> > > 捕获到的变量是以JSON形式进行存储的，而一般情况下用到的是CSV文件，因此本项目提供了一个JSON到CSV的转换工具JsonParser.jar
> > >
> > > 用法：java -jar JsonParser.jar inputFileName outputFileName cols
> > >
> > > 其中， cols为生成csv的列数。
>
> ## TODO
>
> > 做一个更易于使用的JsonParser
>
> > CubeMonitor直接产生csv文件



