#include "bsp_gpio.h"

#include <stdlib.h>

#include "webots/led.h"

static WbDeviceTag led = 0;

inline bsp_status_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value) {
  (void)gpio;

  if (!led) {
    led = wb_robot_get_device("led");
  }

  if (value) {
    wb_led_set(led, 1);
    return BSP_OK;
  } else {
    wb_led_set(led, 0);
    return BSP_OK;
  }

  return BSP_ERR;
}
