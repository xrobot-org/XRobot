/* Includes ------------------------------------------------------------------*/
#include "bsp_usb.h"
#include "bsp_delay.h"

#include "usbd_cdc_if.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t usb_tx_buf[256];
uint8_t usb_rx_buf[256];


/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int BSP_USB_Printf(const char *fmt, ...) {
	static va_list ap;
	uint16_t len = 0;

	va_start(ap, fmt);
	len = vsprintf((char *)usb_tx_buf, fmt, ap);
	va_end(ap);
	
	while(CDC_Transmit_FS(usb_tx_buf, len) != USBD_OK) {
		BSP_Delay(5);
	}
	
	return BSP_USB_OK;
}
