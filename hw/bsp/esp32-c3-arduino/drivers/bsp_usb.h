#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

/* Exported constants ------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

typedef enum {
  BSP_USB_CDC,
  /* BSP_USB_XXX, */
  BSP_USB_NUM,
  BSP_USB_ERR,
} bsp_usb_t;

/* USB支持的中断回调函数类型 */
typedef enum {
  BSP_USB_RX_CPLT_CB,
  BSP_USB_CB_NUM,
} bsp_usb_callback_t;
/* Exported functions prototypes -------------------------------------------- */
bool bsp_usb_connect(void); /* USB已连接 */
size_t bsp_usb_avail(void); /* USB有数据 */
char bsp_usb_read_char();   /* 获取缓存数据 */
size_t bsp_usb_read(uint8_t *buffer, uint32_t len);
bsp_status_t bsp_usb_transmit(const uint8_t *buffer, uint32_t len);
void bsp_usb_init(void);
void bsp_usb_update(void);
#ifdef __cplusplus
}
#endif
