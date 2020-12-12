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
 * @param len 数组长度
 * @param current_max 电机最大电流
 * @return int8_t 0对应没有错误
 */
int8_t PowerLimit_Apply(float power_limit, float vbat, float *motor_out,
                        uint32_t len, uint16_t current_max);
/**
 * @brief 电容输入功率计算
 *
 * @param power_in 底盘当前功率
 * @param power_limit 裁判系统功率限制值
 * @param power_buffer 缓冲能量
 * @return float 裁判系统输出最大值
 */
float PowerLimit_CapInput(float power_in, float power_limit,
                          float power_buffer);

#ifdef __cplusplus
}
#endif
