/*
  自定义的小工具
*/

#include "comp_utils.hpp"

#include "bsp_def.h"

/**
 * @brief 计算平方根倒数
 *
 * @param x 输入
 * @return float 计算结果
 */
float inv_sqrtf(float x) {
#if 0
  /* Fast inverse square-root */
  /* See: http://en.wikipedia.org/wiki/Fast_inverse_square_root */
	float halfx = 0.5f * x;
	float y = x;
	long i = *(long*)&y;
	i = 0x5f3759df - (i>>1);
	y = *(float*)&i;
	y = y * (1.5f - (halfx * y * y));
	y = y * (1.5f - (halfx * y * y));
	return y;
#else
  return 1.0f / sqrtf(x);
#endif
}

/**
 * @brief 将值限制在-limit和limit之间。
 *
 * @param x 输入
 * @param limit 上下界的绝对值
 * @return float 操作后的值
 */
float abs_clampf(float x, float limit) { return MIN(limit, MAX(x, -limit)); }

/**
 * @brief 将值限制在下限和上限之间。
 *
 * @param origin 被操作的值
 * @param lo 下限
 * @param hi 上限
 */
void clampf(float *origin, float lo, float hi) {
  XB_ASSERT(origin);
  *origin = MIN(hi, MAX(*origin, lo));
}

/**
 * @brief 符号函数
 *
 * @param in 输入
 * @return float 运算结果
 */
float signf(float x) {
  if (x == 0.0f) {
    return x;
  } else {
    return (x > 0) ? 1.0f : 0.0f;
  }
}

/**
 * @brief 根据目标弹丸速度计算摩擦轮转速
 *
 * @param bullet_speed 弹丸速度
 * @param fric_radius 摩擦轮半径
 * @param is17mm 是否为17mm
 * @return 摩擦轮转速
 */
float bullet_speed_to_fric_rpm(float bullet_speed, float fric_radius,
                               bool is17mm) {
  if (bullet_speed == 0.0f) {
    return 0.f;
  }
  if (is17mm) {
    if (bullet_speed == 15.0f) {
      return 4670.f;
    }
    if (bullet_speed == 18.0f) {
      return 5200.f;
    }
    if (bullet_speed == 30.0f) {
      return 7400.f;
    }
  } else {
    if (bullet_speed == 10.0f) {
      return 4450.f;
    }
    if (bullet_speed == 16.0f) {
      return 5750.f;
    }
  }

  // TODO:
  /* 不为裁判系统设定值时,计算转速 */
  return 60.0f * bullet_speed / (M_2PI * fric_radius);
}

int float_to_uint(float x, float x_min, float x_max, int bits) {
  /// Converts a float to an unsigned int, given range and number of bits ///
  float span = x_max - x_min;
  float offset = x_min;
  return static_cast<int>((x - offset) * (static_cast<float>((1 << bits) - 1)) /
                          span);
}

float uint_to_float(int x_int, float x_min, float x_max, int bits) {
  /// converts unsigned int to float, given range and number of bits ///
  float span = x_max - x_min;
  float offset = x_min;
  return (static_cast<float>(x_int)) * span /
             (static_cast<float>((1 << bits) - 1)) +
         offset;
}
