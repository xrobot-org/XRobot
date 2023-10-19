#include "bsp_usb.h"

#include <stdint.h>

#include "bsp_def.h"
#include "main.h"

static bsp_callback_t callback_list[BSP_USB_NUM][BSP_USB_CB_NUM];

bsp_status_t bsp_usb_transmit(const uint8_t *buffer, uint32_t len) {
  XB_UNUSED(buffer);
  XB_UNUSED(len);
  return BSP_OK;
}

char bsp_usb_read_char(void) { return 0; }

size_t bsp_usb_read(uint8_t *buffer, uint32_t len) {
  XB_UNUSED(buffer);
  XB_UNUSED(len);
  return 0;
}

bool bsp_usb_connect(void) { return false; }

size_t bsp_usb_avail(void) { return 0; }

void bsp_usb_init() {}

void bsp_usb_update() {}

bsp_status_t bsp_usb_register_callback(bsp_usb_t usb, bsp_usb_callback_t type,
                                       void (*callback)(void *),
                                       void *callback_arg) {
  assert_param(callback);
  assert_param(type != BSP_USB_CB_NUM);

  callback_list[usb][type].fn = callback;
  callback_list[usb][type].arg = callback_arg;
  return BSP_OK;
}
