#include "bsp_gpio.h"

#include <stdlib.h>

#include "bsp_def.h"

inline bsp_status_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value) {
  XB_UNUSED(gpio);
  XB_UNUSED(value);

  return BSP_OK;
}
