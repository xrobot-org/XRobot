#include "bsp_usb.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bsp_delay.h"
#include "comp_utils.h"
#include "tusb.h"

#define BSP_USB_MAX_RX_LEN 1024
#define BSP_USB_MAX_TX_LEN 1024

#define BSP_USB_DEFAULT_SERIAL_PORT 0

uint8_t usb_tx_buf[BSP_USB_MAX_TX_LEN];

static int8_t BSP_USB_Transmit(uint8_t *buffer, uint16_t len) {
  tud_cdc_n_write(BSP_USB_DEFAULT_SERIAL_PORT, buffer, len);
  tud_cdc_n_write_flush(BSP_USB_DEFAULT_SERIAL_PORT);
  return BSP_OK;
}

char BSP_USB_ReadChar(void) {
  char buff;
  uint8_t len = tud_cdc_n_read(BSP_USB_DEFAULT_SERIAL_PORT, &buff, 1);
  if (len == 1)
    return buff;
  else
    return 0;
}

bool BSP_USB_Connect(void) {
  return tud_cdc_n_connected(BSP_USB_DEFAULT_SERIAL_PORT);
}

bool BSP_USB_Avail(void) {
  return tud_cdc_n_available(BSP_USB_DEFAULT_SERIAL_PORT);
}

int8_t BSP_USB_Printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf((char *)usb_tx_buf, BSP_USB_MAX_TX_LEN - 1, fmt, ap);
  va_end(ap);

  if (len > 0) {
    BSP_USB_Transmit(usb_tx_buf, (uint16_t)(len));
    return BSP_OK;
  } else {
    return BSP_ERR_NULL;
  }
}

void OTG_FS_IRQHandler(void) { tud_int_handler(BSP_USB_DEFAULT_SERIAL_PORT); }
