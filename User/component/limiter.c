/*
  限制器
*/

#include "limiter.h"

#include <math.h>
#include <stddef.h>

#define POWER_BUFF_THRESHOLD 20
#define CHASSIS_POWER_CHECK_FREQ 10
#define CHASSIS_POWER_PASS_MULT 0.9f
#define CHASSIS_POWER_NO_PASS__MULT 1.5f

#define CHASSIS_MOTOR_CIRCUMFERENCE 0.12f

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
                                float *speed, uint32_t len) {
  /* power_limit小于0时不进行限制 */
  if (motor_out == NULL || speed == NULL || power_limit < 0) return -1;

  float sum_motor_out = 0.0f;
  for (uint32_t i = 0; i < len; i++) {
    /* 总功率计算 P=F(由转矩电流表示)*V(由转速表示) */
    sum_motor_out +=
        fabsf(motor_out[i]) * fabsf(speed[i]) * CHASSIS_MOTOR_CIRCUMFERENCE;
  }

  /* 保持每个电机输出值缩小时比例不变 */
  if (sum_motor_out > power_limit) {
    for (uint32_t i = 0; i < len; i++) {
      motor_out[i] *= power_limit / sum_motor_out;
    }
  }

  return 0;
}

/**
 * @brief 电容输入功率计算
 *
 * @param power_in 底盘当前功率
 * @param power_limit 裁判系统功率限制值
 * @param power_buffer 缓冲能量
 * @return float 裁判系统输出最大值
 */
float PowerLimit_CapInput(float power_in, float power_limit,
                          float power_buffer) {
  float target_power = 0.0f;

  /* 计算下一个检测周期的剩余缓冲能量 */
  float heat_buff = power_buffer - (float)(power_in - power_limit) /
                                       (float)CHASSIS_POWER_CHECK_FREQ;
  if (heat_buff < POWER_BUFF_THRESHOLD) { /* 功率限制 */
    target_power = power_limit * CHASSIS_POWER_PASS_MULT;
  } else {
    target_power = power_limit * CHASSIS_POWER_NO_PASS__MULT;
  }

  return target_power;
}

/**
 * @brief 使用缓冲能量计算底盘最大功率
 *
 * @param power_limit 裁判系统功率限制值
 * @param power_buffer 缓冲能量
 * @return float 底盘输出最大值
 */
float PowerLimit_TargetPower(float power_limit, float power_buffer) {
  float target_power = 0.0f;

  /* 根据剩余缓冲能量计算输出功率 */
  target_power = power_limit * (power_buffer - 10.0f) / 20.0f;
  if (target_power < 0.0f) target_power = 0.0f;

  return target_power;
}

/**
 *
 * @brief 发射频率控制
 *
 * @param heat 当前热量
 * @param heat_limit 热量上限
 * @param cooling_rate 冷却速率
 * @param heat_increase 冷却增加
 * @param is_big 经过热量限制后的发射频率
 * @return float 发射频率
 */
float HeatLimit_LauncherFreq(float heat, float heat_limit, float cooling_rate,
                             float heat_increase, bool is_big) {
  float heat_percent = heat / heat_limit;
  float stable_freq = cooling_rate / heat_increase;
  if (is_big)
    return stable_freq;
  else
    return (heat_percent > 0.7f) ? stable_freq : 3.0f * stable_freq;
}
