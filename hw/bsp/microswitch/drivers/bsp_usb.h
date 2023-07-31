#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

/* Exported functions prototypes -------------------------------------------- */
bool bsp_usb_connect(void); /* USB已连接 */
size_t bsp_usb_avail(void); /* USB有数据 */
char bsp_usb_read_char();   /* 获取缓存数据 */
size_t bsp_usb_read(uint8_t *buffer, uint32_t len);
bsp_status_t bsp_usb_printf(const char *fmt, ...); /* 打印至虚拟串口 */
bsp_status_t bsp_usb_transmit(const uint8_t *buffer, uint32_t len);
void bsp_usb_init(void);
void bsp_usb_update(void);
#ifdef __cplusplus
}
#endif
