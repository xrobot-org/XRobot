#include "bsp_time.h"

#include "FreeRTOS.h"
#include "main.h"
#include "task.h"

uint32_t bsp_time_get_ms() { return xTaskGetTickCount(); }

uint64_t bsp_time_get_us() {
  uint32_t ms_old = xTaskGetTickCount();
  uint32_t tick_value_old = SysTick->VAL;
  uint32_t ms_new = xTaskGetTickCount();
  uint32_t tick_value_new = SysTick->VAL;
  if (ms_old == ms_new) {
    return ms_new * 1000 + 1000 - tick_value_old * 1000 / (SysTick->LOAD + 1);
  } else {
    return ms_new * 1000 + 1000 - tick_value_new * 1000 / (SysTick->LOAD + 1);
  }
}

uint64_t bsp_time_get() __attribute__((alias("bsp_time_get_us")));
