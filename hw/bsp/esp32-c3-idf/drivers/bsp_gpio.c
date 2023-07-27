#include "bsp_gpio.h"

#include <stdlib.h>

#include "bsp.h"
#include "driver/gpio.h"

inline bsp_status_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value) {
  switch (gpio) {
    case BSP_GPIO_LED:
      gpio_set_level(GPIO_NUM_8, value);
      return BSP_OK;
    default:
      break;
  }

  return BSP_ERR;
}
