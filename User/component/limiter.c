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
 * @param motor 电机输出数组
 * @param last_motor 上次电机输出数组
 * @param len 电机输出数组长度
 * @return int8_t 0对应没有错误
 */
int8_t PowerLimit_Apply(float power_limit, float vbat, float *motor,
                        float *last_motor, uint32_t len) {
  if (motor == NULL) return -1;
  float last_total_current = 0.0f;

  for (uint32_t i = 0; i < len; i++)
    last_total_current += fabsf(last_motor[i]); /* 检测功率是否超出限制 */
  if (power_limit > last_total_current * vbat) return -2;

  float total_current = 0.0f;
  for (uint32_t i = 0; i < len; i++) {
    total_current += fabsf(motor[i]);
  }

  if (power_limit > 0.0f) {
    if ((total_current * vbat) > power_limit) {
      float current_scale =
          power_limit / vbat / total_current; /* 保持每个电机输出值比例不变 */
      for (uint32_t i = 0; i < len; i++) {
        motor[i] *= current_scale;
      }
    }
  }
  return 0;
}
