/* 
	剩余电流算法。
	
	通过电压值计算剩余电量。
*/

#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

#include "user_math.h"

float Capacity_GetBatteryRemain(float volt);
float Capacity_GetCapacitorRemain(float volt);

#ifdef __cplusplus
}
#endif
