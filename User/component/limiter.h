/*
        限制器
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int8_t HeatLimiter_Apply(float heat_limit, float vbat, float dt_sec);
int8_t PowerLimit_Apply(float power_limit, float vbat, float *motor,
                        uint32_t len);

#ifdef __cplusplus
}
#endif
