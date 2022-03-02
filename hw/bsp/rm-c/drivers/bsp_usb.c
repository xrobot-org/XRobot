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

int8_t bsp_usb_transmit(uint8_t *buffer, uint32_t len) {
  tud_cdc_n_write(BSP_USB_DEFAULT_SERIAL_PORT, buffer, len);
  tud_cdc_n_write_flush(BSP_USB_DEFAULT_SERIAL_PORT);
  return BSP_OK;
}

char bsp_usb_read_char(void) {
  char buff;
  uint8_t len = tud_cdc_n_read(BSP_USB_DEFAULT_SERIAL_PORT, &buff, 1);
  if (len == 1)
    return buff;
  else
    return 0;
}

uint32_t bsp_usb_read(uint8_t *buffer, uint32_t len) {
  uint32_t recv_len = tud_cdc_n_read(BSP_USB_DEFAULT_SERIAL_PORT, buffer, len);
  return recv_len;
}

bool bsp_usb_connect(void) {
  return tud_cdc_n_connected(BSP_USB_DEFAULT_SERIAL_PORT);
}

uint32_t bsp_usb_avail(void) {
  return tud_cdc_n_available(BSP_USB_DEFAULT_SERIAL_PORT);
}

int8_t bsp_usb_printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf((char *)usb_tx_buf, BSP_USB_MAX_TX_LEN - 1, fmt, ap);
  va_end(ap);

  if (len > 0) {
    bsp_usb_transmit(usb_tx_buf, (uint16_t)(len));
    return BSP_OK;
  } else {
    return BSP_ERR_NULL;
  }
}

void bsp_usb_init() { tusb_init(); }

void bsp_usb_update() { tud_task(); }

void OTG_FS_IRQHandler(void) { tud_int_handler(BSP_USB_DEFAULT_SERIAL_PORT); }
