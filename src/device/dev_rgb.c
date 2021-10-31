#include "dev_rgb.h"

#include "comp_utils.h"
#include "hal_tim.h"

Err_t RGB_SetColor(ColorHex_t color, LED_Status_t s) {
  uint8_t red = (color >> 16) & 0xFF;
  uint8_t grn = (color >> 8) & 0xFF;
  uint8_t blu = (color >> 0) & 0xFF;
  LED_Set(LED_RED, s, (float)red / (float)UINT8_MAX);
  LED_Set(LED_GRN, s, (float)grn / (float)UINT8_MAX);
  LED_Set(LED_BLU, s, (float)blu / (float)UINT8_MAX);
  return RM_OK;
}
