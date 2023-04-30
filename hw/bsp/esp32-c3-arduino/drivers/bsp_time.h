#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

uint32_t bsp_time_get_ms();

uint32_t bsp_time_get_us();

float bsp_time_get();

#ifdef __cplusplus
}
#endif
