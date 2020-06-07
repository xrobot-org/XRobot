/* 

*/

#pragma once

#include <stdint.h>

int HeatLimiter_Apply(float heat_limit, float vbat, float dt_sec);
int PowerLimit_Apply(float power_limit, float vbat, float *motor, uint32_t len);
