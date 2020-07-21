/* 
	剩余电流算法。
	
	通过电压值计算剩余电量。
*/

#pragma once

#include "user_math.h"

float Capacity_GetBatteryRemain(float volt);
float Capacity_GetCapacitorRemain(float volt);
