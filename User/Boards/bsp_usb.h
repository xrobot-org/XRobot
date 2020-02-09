#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

extern uint8_t usb_rx_buf[2048];

/* Exported functions prototypes ---------------------------------------------*/
int BSP_USB_Printf(const char *fmt, ...);
