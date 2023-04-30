#include "FreeRTOS.h"
#include "bsp.h"
#include "robot.hpp"

#undef LOW

#include "Arduino.h"
#include "main.ino"

extern "C" void app_main(void) {
  initArduino();
  setup();
  bsp_init();
  robot_init();
  while (1) {
    loop();
  }
}
