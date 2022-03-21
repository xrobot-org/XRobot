#include "bsp_usb.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "bsp_delay.h"
#include "comp_utils.h"
#include "tusb.h"

#define BSP_USB_MAX_RX_LEN 1024
#define BSP_USB_MAX_TX_LEN 1024

static bsp_callback_t callback_list[BSP_USB_NUM][BSP_USB_CB_NUM];

uint8_t usb_tx_buf[BSP_USB_MAX_TX_LEN];

int8_t bsp_usb_transmit(uint8_t *buffer, uint32_t len) {
  tud_cdc_write(buffer, len);
  tud_cdc_write_flush();
  return BSP_OK;
}

char bsp_usb_read_char(void) {
  char buff;
  uint8_t len = tud_cdc_read(&buff, 1);
  if (len == 1)
    return buff;
  else
    return 0;
}

uint32_t bsp_usb_read(uint8_t *buffer, uint32_t len) {
  uint32_t recv_len = tud_cdc_read(buffer, len);
  return recv_len;
}

bool bsp_usb_connect(void) { return tud_cdc_connected(); }

uint32_t bsp_usb_avail(void) { return tud_cdc_available(); }

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

static void bsp_usb_callback(bsp_usb_callback_t cb_type, bsp_usb_t bsp_usb) {
  if (bsp_usb != BSP_USB_ERR) {
    bsp_callback_t cb = callback_list[bsp_usb][cb_type];

    if (cb.fn) {
      cb.fn(cb.arg);
    }
  }
}

int8_t bsp_usb_register_callback(bsp_usb_t usb, bsp_usb_callback_t type,
                                 void (*callback)(void *), void *callback_arg) {
  ASSERT(callback);
  ASSERT(type != BSP_USB_CB_NUM);

  callback_list[usb][type].fn = callback;
  callback_list[usb][type].arg = callback_arg;
  return BSP_OK;
}

void OTG_FS_IRQHandler(void) { tud_int_handler(0); }

void tud_cdc_rx_cb(uint8_t itf) { bsp_usb_callback(BSP_USB_RX_CPLT_CB, itf); }
