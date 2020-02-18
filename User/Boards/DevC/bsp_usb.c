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
/* Must set to NULL explicitly. */
osThreadId gbsp_usb_alert = NULL;
uint16_t usb_rx_num = 0;
uint8_t usb_rx_buf[BSP_USB_MAX_RX_LEN];
uint8_t usb_tx_buf[BSP_USB_MAX_TX_LEN];

/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int BSP_USB_Init(osThreadId alert) {
	
	gbsp_usb_alert = alert;
	
	return BSP_USB_OK;
}

int BSP_USB_ReadyReceive(void) {
	CDC_ReadyReceive();
	memset(usb_rx_buf, 0, usb_rx_num);
	return BSP_USB_OK;
}

char BSP_USB_ReadChar(void) {
	return usb_rx_buf[0];
}

int BSP_USB_Transmit(uint8_t *buffer, uint16_t len) {
	while(CDC_Transmit_FS(buffer, len) != USBD_OK) {
		BSP_Delay(5);
	}
	return BSP_USB_OK;
}

int BSP_USB_Printf(const char *fmt, ...) {
	static va_list ap;
	uint16_t len = 0;

	va_start(ap, fmt);
	len = vsprintf((char *)usb_tx_buf, fmt, ap);
	va_end(ap);
	
	BSP_USB_Transmit(usb_tx_buf, len);
	
	return BSP_USB_OK;
}
