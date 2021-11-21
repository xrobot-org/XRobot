#pragma once

#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "bsp.h"
#include "task.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported functions prototypes -------------------------------------------- */
int8_t BSP_USB_StartReceive(void); /* 指示USB库准备好接收数据 */
char BSP_USB_ReadChar(void);       /* 读取缓存第一个字符 */

int8_t BSP_USB_Printf(const char *fmt, ...); /* 打印至虚拟串口 */
