#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>

#include <cmsis_os2.h>

#include "bsp/bsp.h"

/* Exported constants --------------------------------------------------------*/
#define BSP_USB_MAX_RX_LEN	128
#define BSP_USB_MAX_TX_LEN	1024

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

extern osThreadId_t gbsp_usb_alert;
extern uint8_t usb_rx_buf[BSP_USB_MAX_RX_LEN];
extern uint8_t usb_tx_buf[BSP_USB_MAX_TX_LEN];
/* Exported functions prototypes ---------------------------------------------*/
int8_t BSP_USB_ReadyReceive(osThreadId_t alert);
char BSP_USB_ReadChar(void);

int8_t BSP_USB_Printf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
