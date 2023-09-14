#include "bsp.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bsp_pwm.h"
#include "bsp_time.h"
#include "bsp_uart.h"

void bsp_init() {
  if (access("/dev/ttyCH343USB0", W_OK)) {
    system(
        "$(echo 1 > /sys/class/leds/usb_enable/brightness && sleep 3 && echo 0 "
        "> /sys/class/leds/usb_enable/brightness && sleep 3)");
  }

  bsp_uart_init();
  bsp_time_init();
  bsp_pwm_init();
}
