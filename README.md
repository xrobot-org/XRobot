# RoboMaster

Code of MCU for RoboMaster. UNFINISHED & UNTESTED.

## TODO

- Port LittleFS.
- Add logging and file on top of LittleFS.
- Add gimbal soft limit.
- Get a better name for BSP, maybe "mcu".
- Add mutex to BSP USB print.
  - Add mutex to all device and BSP.
  - device.c里面加上一个Device_Init()，在里面初始化所有mutex
- CAN设备代码优化。消息解析发送方向。
  - CAN设备动态初始化，保存好几组配置。

UNFINISHED & UNTESTED.
