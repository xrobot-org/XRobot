#include "bsp_time.h"

#include "FreeRTOS.h"
#include "main.h"
#include "task.h"

uint32_t bsp_time_get_ms() { return xTaskGetTickCount(); }

uint64_t bsp_time_get_us() {
  return xTaskGetTickCount() * 1000 + 1000 -
         (SysTick->VAL * 1000 / SysTick->LOAD);
}

uint64_t bsp_time_get() __attribute__((alias("bsp_time_get_us")));
