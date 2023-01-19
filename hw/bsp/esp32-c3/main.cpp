// #include <cstdint>

#include "FreeRTOS.h"
#include "bsp.h"
#include "robot.hpp"

extern "C" void app_main(void) {
  bsp_init();
  robot_init();
  while (1) {
    vTaskDelay(UINT32_MAX);
  }
}
