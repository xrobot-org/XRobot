/* 
	限制器
*/

#pragma once

#include <stdint.h>

#include "user_math.h"

int8_t HeatLimiter_Apply(float32_t heat_limit, float32_t vbat, float32_t dt_sec);
int8_t PowerLimit_Apply(float32_t power_limit, float32_t vbat, float32_t *motor, uint32_t len);
