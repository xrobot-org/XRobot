#include "bsp_time.h"

#include "main.h"

uint32_t bsp_time_get_ms() { return HAL_GetTick(); }

uint32_t bsp_time_get_us() {
  return HAL_GetTick() * 1000 + 1000 - (SysTick->VAL * 1000 / SysTick->LOAD);
}

float bsp_time_get() {
  return (float)((HAL_GetTick() * 1000 + 1000 -
                  (SysTick->VAL * 1000 / SysTick->LOAD)) /
                 1000000.0f);
}
