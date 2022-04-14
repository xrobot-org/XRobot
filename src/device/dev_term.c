#include "dev_term.h"

#include "FreeRTOS.h"
#include "bsp_usb.h"
#include "queue.h"
#include "semphr.h"

static bool inited = false;

err_t term_init() {
  if (inited) return RM_OK;

  bsp_usb_init();
  inited = true;
  return RM_OK;
}

inline err_t term_update() {
  bsp_usb_update();
  return RM_OK;
}

inline bool term_opened() { return bsp_usb_connect(); }

inline uint32_t term_avail() { return bsp_usb_avail(); }

inline char term_read_char() { return bsp_usb_read_char(); }

inline uint16_t term_read(uint8_t *buffer, uint32_t len) {
  return bsp_usb_read(buffer, len);
}

inline err_t term_write(uint8_t *buffer, uint32_t len) {
  return bsp_usb_transmit(buffer, len);
}
