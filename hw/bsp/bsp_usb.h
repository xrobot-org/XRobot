#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bsp.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported functions prototypes -------------------------------------------- */
bool bsp_usb_connect(void);                  /* USB已连接 */
bool bsp_usb_avail(void);                    /* USB有数据 */
char bsp_usb_read_char();                     /* 获取缓存数据 */
int8_t bsp_usb_printf(const char *fmt, ...); /* 打印至虚拟串口 */
void bsp_usb_init(void);
void bsp_usb_update(void);
