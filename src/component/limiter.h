/*
  限制器
*/

#pragma once

#include <stdbool.h>
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
 * @brief 使用缓冲能量计算底盘最大功率
 *
 * @param power_limit 裁判系统功率限制值
 * @param power_buffer 缓冲能量
 * @return float 底盘输出最大值
 */
float PowerLimit_TargetPower(float power_limit, float power_buffer);

/**
 * @brief 发射频率控制
 *
 * @param heat 当前热量
 * @param heat_limit 热量上限
 * @param cooling_rate 冷却速率
 * @param heat_increase 冷却增加
 * @param is_big 是否为大弹丸
 * @return float 发射频率
 */
float HeatLimit_LauncherFreq(float heat, float heat_limit, float cooling_rate,
                             float heat_increase, bool is_big);
