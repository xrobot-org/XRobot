#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

uint32_t bsp_get_random_num(void);
int32_t bsp_get_random_rangle(int32_t min, int32_t max);

#ifdef __cplusplus
}
#endif
