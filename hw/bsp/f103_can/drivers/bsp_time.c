#include "bsp_time.h"

#include "main.h"

uint32_t bsp_time_get_ms() { return HAL_GetTick(); }

uint32_t bsp_time_get_us() { return HAL_GetTick() * 1000; }

float bsp_time_get() { return ((float)HAL_GetTick() / 1000); }
