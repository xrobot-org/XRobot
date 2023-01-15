#include "bsp_gpio.h"

#include <stdlib.h>

inline int8_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value) {
  (void)gpio;

  if (value) {
    if (!system("spd-say -r 100 -p 100 -R 100  滴")) {
      return BSP_OK;
    }
  }

  return BSP_ERR;
}
