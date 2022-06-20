#include "dev_term.hpp"

#include "FreeRTOS.h"
#include "bsp_usb.h"
#include "queue.h"
#include "semphr.h"

static bool inited = false;

int8_t term_init() {
  if (inited) return RM_OK;

  bsp_usb_init();
  inited = true;
  return RM_OK;
}

int8_t term_update() {
  bsp_usb_update();
  return RM_OK;
}

bool term_opened() { return bsp_usb_connect(); }

uint32_t term_avail() { return bsp_usb_avail(); }

char term_read_char() { return bsp_usb_read_char(); }

uint16_t term_read(uint8_t *buffer, uint32_t len) {
  return bsp_usb_read(buffer, len);
}

int8_t term_write(uint8_t *buffer, uint32_t len) {
  return (int8_t)bsp_usb_transmit(buffer, len);
}
