/*
  限制器
*/

#include "limiter.h"

#include <math.h>
#include <stddef.h>

int8_t HeatLimiter_Apply(float heat_limit, float vbat, float dt_sec) {
  (void)heat_limit;
  (void)vbat;
  (void)dt_sec;
  return 0;
}

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
                        float *cur_fb, uint32_t len) {
  if (motor_out == NULL) return -1;
  float last_total_current = 0.0f;

  for (uint32_t i = 0; i < len; i++)
    last_total_current += fabsf(cur_fb[i]); /* 检测功率是否超出限制 */
  if (power_limit > last_total_current * vbat) return -2;

  float total_current = 0.0f;
  for (uint32_t i = 0; i < len; i++) {
    total_current += fabsf(motor_out[i]);
  }

  if (power_limit > 0.0f) {
    if ((total_current * vbat) > power_limit) {
      float current_scale =
          power_limit / vbat / total_current; /* 保持每个电机输出值比例不变 */
      for (uint32_t i = 0; i < len; i++) {
        motor_out[i] *= current_scale;
      }
    }
  }
  return 0;
}
