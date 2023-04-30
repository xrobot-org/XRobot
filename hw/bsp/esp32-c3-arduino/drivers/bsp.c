#include "bsp.h"

#include "driver/gpio.h"

void bsp_init() {
  gpio_reset_pin(GPIO_NUM_8);
  gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
}
