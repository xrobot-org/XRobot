# File Structure

| 文件夹 | 内容 |
| ---- | ----  |
| Core | CubeMX生成。包含核心代码，外设初始化，系统初始化等。|
| Doc | 包含文档。|
| Drivers | CubeMX生成。包含CMSIS相关库和STM32 HAL。|
| MDK-ARM | CubeMX生成。Keil uversion 项目相关文件。|
| Middlewares | 部分CubeMX生成，部分手动添加。第三方中间件。|
| USB_DEVICE | CubeMX生成。USB相关文件。 |
| User | 用户编文件。 |

| User内 | 内容 |
| ---- | ----  |
| bsp | 文件夹包含开发板信息，基于STM32 HAL对板载的外设进行控制。|
| component | 包含各种组件，自成一体，不应该依赖于任何其他文件夹。|
| device | 独立于板子的设备，依赖于HAL和bsp。|
| module | 对机器人各部分的抽象，各模块一起组成机器人。|
| task | 一个个独立的任务：通信、姿态解算、控制各个模组。|
