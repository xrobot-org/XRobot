/* Includes ------------------------------------------------------------------*/
#include "bsp\usb.h"
#include "bsp\delay.h"

#include "usbd_cdc_if.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Must set to NULL explicitly. */
osThreadId_t gbsp_usb_alert = NULL;
uint8_t usb_rx_buf[BSP_USB_MAX_RX_LEN];
uint8_t usb_tx_buf[BSP_USB_MAX_TX_LEN];


/* Private function  ---------------------------------------------------------*/
static int8_t BSP_USB_Transmit(uint8_t *buffer, uint16_t len) {
	while(CDC_Transmit_FS(buffer, len) != USBD_OK) {
		BSP_Delay(10);
	}
	
	return BSP_OK;
}

/* Exported functions --------------------------------------------------------*/
int8_t BSP_USB_ReadyReceive(osThreadId_t alert) {
	if (alert == NULL)
		return BSP_ERR_NULL;
	
	gbsp_usb_alert = alert;
	CDC_ReadyReceive();
	return BSP_OK;
}

char BSP_USB_ReadChar(void) {
	return usb_rx_buf[0];
}

int8_t BSP_USB_Printf(const char *fmt, ...) {
	va_list ap;
	uint16_t len = 0;
	
	va_start(ap, fmt);
	len = (uint16_t)vsnprintf((char*)usb_tx_buf, BSP_USB_MAX_TX_LEN - 1, fmt, ap);
	va_end(ap);
	
	if (len > 0) {
		BSP_USB_Transmit(usb_tx_buf, len);
		return BSP_OK;
	} else {
		return BSP_ERR_NULL;
	}
	
}
