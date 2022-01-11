#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "FreeRTOS.h"
#include "bsp.h"
#include "task.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported functions prototypes -------------------------------------------- */
bool BSP_USB_Connect(void);                  /* USB已连接 */
bool BSP_USB_Avail(void);                    /* USB有数据 */
char BSP_USB_ReadChar();                     /* 获取缓存数据 */
int8_t BSP_USB_Printf(const char *fmt, ...); /* 打印至虚拟串口 */
