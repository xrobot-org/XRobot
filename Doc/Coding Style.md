# Coding Style

typedef 定义的类后面加_t，例：PID_t
用 #pragma once来保证头文件唯一性。

SPI配置使用full duplex 模式，即使只DMA接受，也需要配置DMA发送。
