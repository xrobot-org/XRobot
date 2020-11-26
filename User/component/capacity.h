/*
  剩余电量算法。

  通过电压值计算剩余电量。
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "user_math.h"

/**
 * @brief 通过电压计算电池剩余电量
 * 
 * @param volt 电压值
 * @return float 剩余电量比例
 */
float Capacity_GetBatteryRemain(float volt);

/**
 * @brief 通过电压计算电容剩余电量
 * 
 * @param volt 电压值
 * @return float 剩余电量比例
 */
float Capacity_GetCapacitorRemain(float volt);

#ifdef __cplusplus
}
#endif
