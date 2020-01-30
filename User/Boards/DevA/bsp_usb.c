/* Includes ------------------------------------------------------------------*/
#include "bsp_usb.h"

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "usbd_cdc_if.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t usb_buf[256];

/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int Board_USBPrint(const char *fmt,...) {
	static va_list ap;
	uint16_t len = 0;

	va_start(ap, fmt);
	len = vsprintf((char *)usb_buf, fmt, ap);
	va_end(ap);

	CDC_Transmit_FS(usb_buf, len);
	
	return 0;
}
