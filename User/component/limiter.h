/*
  限制器
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int8_t HeatLimiter_Apply(float heat_limit, float vbat, float dt_sec);

/**
 * @brief 底盘功率限制
 * TODO：变量改名
 * 
 * @param power_limit 功率阈值
 * @param vbat 电池电压
 * @param motor 电机输出数组
 * @param last_motor 上次电机输出数组
 * @param len 电机输出数组长度
 * @return int8_t 0对应没有错误
 */
int8_t PowerLimit_Apply(float power_limit, float vbat, float *motor,
                        float *last_motor, uint32_t len);

#ifdef __cplusplus
}
#endif
