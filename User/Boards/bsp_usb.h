#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>

/* Exported constants --------------------------------------------------------*/
#define BSP_USB_OK			(0)
#define BSP_USB_TIMEOUT		(-2)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
int BSP_USB_Printf(const char *fmt, ...);
