#include "bsp_gpio.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

static int dev = -1;
static const char ON = '1', OFF = '0';

inline bsp_status_t bsp_gpio_write_pin(bsp_gpio_t gpio, bool value) {
  (void)gpio;

  if (dev == -1) {
    dev = open("/sys/class/leds/blink_led/brightness",
               O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
  }

  if (value) {
    write(dev, &ON, sizeof(ON));
  } else {
    write(dev, &OFF, sizeof(OFF));
  }

  return BSP_ERR;
}
