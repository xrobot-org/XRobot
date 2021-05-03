/*
  剩余电量算法。

  通过电压值计算剩余电量。
*/

#include "capacity.h"

/**
 * @brief 通过电压计算电池剩余电量
 *
 * @param vbat 电池电压值
 * @return float 剩余电量比例
 */
float Capacity_GetBatteryRemain(float vbat) {
  float percentage;
  float volt_2 = vbat * vbat;
  float volt_3 = volt_2 * vbat;

  if (vbat < 19.5f)
    percentage = 0.0f;

  else if (vbat < 21.9f)
    percentage = 0.005664f * volt_3 - 0.3386f * volt_2 + 6.765f * vbat - 45.17f;

  else if (vbat < 25.5f)
    percentage = 0.02269f * volt_3 - 1.654f * volt_2 + 40.34f * vbat - 328.4f;

  else
    percentage = 1.0f;

  if (percentage < 0.0f)
    percentage = 0.0f;

  else if (percentage > 1.0f)
    percentage = 1.0f;

  return percentage;
}

/**
 * @brief
 *
 * @param vcap 电容电压
 * @param vbat 电池电压
 * @param v_cutoff 截止电压
 * @return float 电容剩余电量比例
 */
float Capacity_GetCapacitorRemain(float vcap, float vbat, float v_cutoff) {
  /* 根据公式E=1/2CU^2，省略常数项 */
  const float c_max = vbat * vbat;
  const float c_cap = vcap * vcap;
  const float c_min = v_cutoff * v_cutoff;
  float percentage = (c_cap - c_min) / (c_max - c_min);
  Clamp(&percentage, 0.0f, 1.0f);
  return percentage;
}
