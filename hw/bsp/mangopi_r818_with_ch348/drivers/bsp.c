#include "bsp.h"

#include "bsp_time.h"
#include "bsp_uart.h"

void bsp_init() {
  system(
      "ls /dev/ttyCH9344USB0 || $(echo 1 > "
      "/sys/class/leds/usb_enable/brightness && "
      "sleep 3 && echo 0 > "
      "/sys/class/leds/usb_enable/brightness "
      "&& sleep 3)");
  bsp_uart_init();
  bsp_time_init();
}
