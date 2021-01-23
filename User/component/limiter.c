/*
  限制器
*/

#include "limiter.h"

#include <math.h>
#include <stddef.h>

#define POWER_BUFF_THRESHOLD 20
#define CHASSIS_POWER_CHECK_FREQ 10
#define CHASSIS_POWER_FACTOR_PASS 0.9f
#define CHASSIS_POWER_FACTOR_NO_PASS 1.5f

/**
 * @brief 底盘功率限制
 *
 * @param power_limit 功率阈值
 * @param vbat 电池电压
 * @param motor_out 电机输出值数组
 * @param len 数组长度
 * @param current_max 电调最大输出电流
 * @return int8_t 0对应没有错误
 */
int8_t PowerLimit_Apply(float power_limit, float vbat, float *motor_out,
                        uint32_t len, uint16_t current_max) {
  if (motor_out == NULL) return -1;

  float sum_motor_out = 0.0f;
  for (uint32_t i = 0; i < len; i++) {
    sum_motor_out += fabsf(motor_out[i]);
  }

  if (power_limit > 0.0f) {
    if ((sum_motor_out * vbat * current_max) > power_limit) {
      for (uint32_t i = 0; i < len; i++) {
        motor_out[i] *= power_limit / (sum_motor_out * vbat * current_max);
      }
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

  float heat_buff = power_buffer - (float)(power_in - power_limit) /
                                       (float)CHASSIS_POWER_CHECK_FREQ;
  if (heat_buff < POWER_BUFF_THRESHOLD) { /* 功率限制 */
    target_power = power_limit * CHASSIS_POWER_FACTOR_PASS;
  } else {
    target_power = power_limit * CHASSIS_POWER_FACTOR_NO_PASS;
  }

  return target_power;
}

/**
 * @brief 射击频率控制
 *
 * @param heat_percent 当前热量与热量上限的比值
 * @param stable_freq_hz 使机器人射击但热量不变化的射击频率
 * @param shoot_freq_hz cmd.c预设的频率
 * @return 经过热量限制后的射击频率
 */
float HeatLimit_ShootFreq(float heat_percent, float stable_freq_hz,
                          float shoot_freq_hz) {
  if (heat_percent <= 0.5) {
    if (shoot_freq_hz > stable_freq_hz * 1.5)
      return stable_freq_hz * 1.5;
    else
      return shoot_freq_hz;
  } else if (heat_percent <= 0.8) {
    if (shoot_freq_hz > stable_freq_hz * 1.1)
      return stable_freq_hz * 1.1;
    else
      return shoot_freq_hz;
  } else if (heat_percent <= 0.9) {
    if (shoot_freq_hz > stable_freq_hz * 0.8)
      return stable_freq_hz * 0.8;
    else
      return shoot_freq_hz;
  } else
    return 0;
}
