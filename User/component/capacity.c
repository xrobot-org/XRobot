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
  const float kTB47S_CELL = 6.0f;
  if (vbat > (4.2f * kTB47S_CELL)) {
    return 1.0f;
  } else {
    float hi, lo, base, step;
    if (vbat > (4.06f * kTB47S_CELL)) {
      hi = 4.2f;
      lo = 4.06f;
      base = 0.9f;
      step = 0.1f;
    } else if (vbat > (4.0f * kTB47S_CELL)) {
      hi = 4.06f;
      lo = 4.0f;
      base = 0.8f;
      step = 0.1f;
    } else if (vbat > (3.93f * kTB47S_CELL)) {
      hi = 4.0f;
      lo = 3.93f;
      base = 0.7f;
      step = 0.1f;
    } else if (vbat > (3.87f * kTB47S_CELL)) {
      hi = 3.93f;
      lo = 3.87f;
      base = 0.6f;
      step = 0.1f;
    } else if (vbat > (3.82f * kTB47S_CELL)) {
      hi = 3.87f;
      lo = 3.82f;
      base = 0.5f;
      step = 0.1f;
    } else if (vbat > (3.79f * kTB47S_CELL)) {
      hi = 3.82f;
      lo = 3.79f;
      base = 0.4f;
      step = 0.1f;
    } else if (vbat > (3.77f * kTB47S_CELL)) {
      hi = 3.79f;
      lo = 3.77f;
      base = 0.3f;
      step = 0.1f;
    } else if (vbat > (3.73f * kTB47S_CELL)) {
      hi = 3.77f;
      lo = 3.73f;
      base = 0.2f;
      step = 0.1f;
    } else if (vbat > (3.7f * kTB47S_CELL)) {
      hi = 3.73f;
      lo = 3.7f;
      base = 0.15f;
      step = 0.05f;
    } else if (vbat > (3.68f * kTB47S_CELL)) {
      hi = 3.7f;
      lo = 3.68f;
      base = 0.1f;
      step = 0.05f;
    } else if (vbat > (3.5f * kTB47S_CELL)) {
      hi = 3.68f;
      lo = 3.5f;
      base = 0.05f;
      step = 0.05f;
    } else if (vbat > (2.5f * kTB47S_CELL)) {
      hi = 3.5f;
      lo = 2.5f;
      base = 0.0f;
      step = 0.05f;
    } else {
      return 0.0f;
    }
    return base + step / (hi - lo) * (vbat / kTB47S_CELL - lo);
  }
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
