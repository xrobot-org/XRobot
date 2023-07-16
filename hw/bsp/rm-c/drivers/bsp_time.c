#include "bsp_time.h"

#include "FreeRTOS.h"
#include "main.h"
#include "task.h"

uint32_t bsp_time_get_ms() { return xTaskGetTickCount(); }

uint32_t bsp_time_get_us() {
  return xTaskGetTickCount() * 1000 + 1000 -
         (SysTick->VAL * 1000 / SysTick->LOAD);
}

float bsp_time_get() {
  return (float)((xTaskGetTickCount() * 1000 + 1000 -
                  (SysTick->VAL * 1000 / SysTick->LOAD)) /
                 1000000.0f);
}
