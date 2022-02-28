#include "dev_term.h"

#include "FreeRTOS.h"
#include "bsp_usb.h"
#include "queue.h"
#include "semphr.h"

static SemaphoreHandle_t term_ctrl;

err_t term_init() {
  bsp_usb_init();
  term_ctrl = xSemaphoreCreateBinary();
  xSemaphoreGive(term_ctrl);
  return RM_OK;
}

inline err_t term_update() {
  bsp_usb_update();
  return RM_OK;
}

inline bool term_opened() { return bsp_usb_connect(); }

inline bool term_avail() { return bsp_usb_avail(); }

inline char term_read_char() { return bsp_usb_read_char(); }

inline err_t term_get_ctrl(uint32_t timeout) {
  return xSemaphoreTake(term_ctrl, timeout) == pdTRUE;
}

inline err_t term_give_ctrl() { return xSemaphoreGive(term_ctrl) == pdTRUE; }
