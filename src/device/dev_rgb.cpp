#include "dev_rgb.hpp"

int8_t rgb_set_color(color_hex_t color, led_status_t s) {
  uint8_t red = (color >> 16) & 0xFF;
  uint8_t grn = (color >> 8) & 0xFF;
  uint8_t blu = (color >> 0) & 0xFF;
  led_set(LED_RED, s, (float)red / (float)UINT8_MAX);
  led_set(LED_GRN, s, (float)grn / (float)UINT8_MAX);
  led_set(LED_BLU, s, (float)blu / (float)UINT8_MAX);
  return RM_OK;
}
