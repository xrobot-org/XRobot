/*
  限制器
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief 限制底盘功率不超过power_limit
 *
 * @param power_limit 最大功率
 * @param motor_out 电机输出值
 * @param speed 电机转速
 * @param len 电机数量
 * @return int8_t 0对应没有错误
 */
int8_t PowerLimit_ChassicOutput(float power_limit, float *motor_out,
                                float *speed, uint32_t len);
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
/**
 * @brief 底盘功率计算
 *
 * @param power_limit 裁判系统功率限制值
 * @param power_buffer 缓冲能量
 * @return float 底盘输出最大值
 */
float PowerLimit_TargetPower(float power_limit, float power_buffer);

/**
 * @brief 射击频率控制
 *
 * @param heat_percent 当前热量与热量上限的比值
 * @param stable_freq_hz 使机器人射击但热量不变化的射击频率
 * @param shoot_freq_hz cmd.c预设的频率
 * @return 经过热量限制后的射击频率
 */
float HeatLimit_ShootFreq(float heat_percent, float stable_freq_hz,
                          float shoot_freq_hz);
