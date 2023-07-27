#include "bsp_usb.h"

#include "FreeRTOS.h"
#include "main.h"
#include "task.h"
#include "tusb.h"

static bsp_callback_t callback_list[BSP_USB_NUM][BSP_USB_CB_NUM];

bsp_status_t bsp_usb_transmit(const uint8_t *buffer, uint32_t len) {
  while (1) {
    uint32_t avil = tud_cdc_write_available();
    if (avil > len) {
      tud_cdc_write(buffer, len);
      tud_cdc_write_flush();
      break;
    } else {
      tud_cdc_write(buffer, avil);
      tud_cdc_write_flush();
      buffer += avil;
      len -= avil;
      vTaskDelay(1);
    }
  }
  return BSP_OK;
}

char bsp_usb_read_char(void) {
  char buff = 0;
  uint8_t len = tud_cdc_read(&buff, 1);
  if (len == 1) {
    return buff;
  } else {
    return 0;
  }
}

size_t bsp_usb_read(uint8_t *buffer, uint32_t len) {
  size_t recv_len = tud_cdc_read(buffer, len);
  return recv_len;
}

bool bsp_usb_connect(void) { return tud_cdc_connected(); }

size_t bsp_usb_avail(void) { return tud_cdc_available(); }

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

bsp_status_t bsp_usb_register_callback(bsp_usb_t usb, bsp_usb_callback_t type,
                                       void (*callback)(void *),
                                       void *callback_arg) {
  assert_param(callback);
  assert_param(type != BSP_USB_CB_NUM);

  callback_list[usb][type].fn = callback;
  callback_list[usb][type].arg = callback_arg;
  return BSP_OK;
}

void OTG_FS_IRQHandler(void) { tud_int_handler(0); }

void tud_cdc_rx_cb(uint8_t itf) { bsp_usb_callback(BSP_USB_RX_CPLT_CB, itf); }
