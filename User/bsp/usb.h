#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>

#include <cmsis_os2.h>

/* Exported constants --------------------------------------------------------*/
#define BSP_USB_OK			(0)
#define BSP_USB_ERR_NULL	(-1)
#define BSP_USB_ERR_INITED	(-2)
#define BSP_USB_TIMEOUT		(-3)

#define BSP_USB_SIGNAL_BUF_RECV (1u<<0)

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
