#include <cstdio>
#include <thread.hpp>

#include "bsp.h"
#include "robot.hpp"

int devr, devg, devb, mode = 0;
uint8_t lightr, lightg, lightb;
char buff[10];

int main() {
  bsp_init();
  robot_init();
  devr = open("/sys/class/leds/sunxi_led0r/brightness",
              O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
  devg = open("/sys/class/leds/sunxi_led0g/brightness",
              O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
  devb = open("/sys/class/leds/sunxi_led0b/brightness",
              O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
  while (1) {
    poll(NULL, 0, 4);
    snprintf(buff, sizeof(buff), "%d", lightr);
    write(devr, buff, sizeof(strlen(buff)));
    snprintf(buff, sizeof(buff), "%d", lightg);
    write(devg, buff, sizeof(strlen(buff)));
    snprintf(buff, sizeof(buff), "%d", lightb);
    write(devb, buff, sizeof(strlen(buff)));
    switch (mode) {
      case 0:
        lightb = 0;
        lightr++;
        lightg = 255 - lightr;
        if (lightr == 255) {
          mode = 2;
        }
        break;
      case 1:
        lightr = 0;
        lightg++;
        lightb = 255 - lightg;
        if (lightg == 255) {
          mode = 0;
        }
        break;
      case 2:
        lightg = 0;
        lightb++;
        lightr = 255 - lightb;
        if (lightb == 255) {
          mode = 1;
        }
        break;
      default:
        break;
    }
  }
}
