#include "bsp.h"

#include "bsp_time.h"
#include "bsp_uart.h"
#include "driver/gpio.h"

void bsp_init() {
  bsp_time_init();
  gpio_reset_pin(GPIO_NUM_8);
  gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
  bsp_uart_init();
}
