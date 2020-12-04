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
 *
 * @param power_limit 功率阈值
 * @param vbat 电池电压
 * @param motor_out 电机输出值数组
 * @param cur_fb 电机电流反馈值数组
 * @param len 数组长度
 * @return int8_t 0对应没有错误
 */
int8_t PowerLimit_Apply(float power_limit, float vbat, float *motor_out,
                        float *cur_fb, uint32_t len);

float PowerLimit_CapInput(float chassis_power, float chassis_power_limit,
                          float chassis_power_buffer);

#ifdef __cplusplus
}
#endif
