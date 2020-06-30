/* 
	剩余电流算法。
	
	通过电压值计算剩余电量。
*/

#pragma once

#include "user_math.h"

float32_t Capacity_GetBatteryRemain(float32_t voltage);
float32_t Capacity_GetCapacitorRemain(float32_t voltage);
