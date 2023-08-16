#include "bsp_time.h"

#include "main.h"

uint32_t bsp_time_get_ms() { return HAL_GetTick(); }

uint64_t bsp_time_get_us() {
  return HAL_GetTick() * 1000 + 1000 - (SysTick->VAL * 1000 / SysTick->LOAD);
}

uint64_t bsp_time_get() __attribute__((alias("bsp_time_get_us")));
