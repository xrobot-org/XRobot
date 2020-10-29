/*
  自定义的数学运算。
*/

#include "user_math.h"

#include <string.h>

inline float InvSqrt(float x) {
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

inline float AbsClip(float in, float limit) {
  return (in < -limit) ? -limit : ((in > limit) ? limit : in);
}

inline float Sign(float in) { return (in > 0) ? 1.0f : 0.0f; }

/*!
 * \brief 将运动向量置零
 *
 * \param mv 被操作的值
 */
inline void ResetMoveVector(MoveVector_t *mv) {
  memset(mv, 0, sizeof(MoveVector_t));
}

/*!
 * \brief 计算循环值的误差，用于没有负数值，并在一定范围内变化的值
 * 例如编码器：相差1.5PI其实等于相差-0.5PI
 *
 * \param sp 被操作的值
 * \param fb 变化量
 * \param range 被操作的值变化范围，正数时起效
 *
 * \return 函数运行结果
 */
inline float CircleError(float sp, float fb, float range) {
  float error = sp - fb;
  if (range > 0.0f) {
    float half_range = range / 2.0f;

    if (error > half_range)
      error -= range;
    else if (error < -half_range)
      error += range;
  }
  return error;
}

/*!
 * \brief 循环加法，用于没有负数值，并在一定范围内变化的值
 * 例如编码器，在0-2PI内变化，1.5PI + 1.5PI = 1PI
 *
 * \param origin 被操作的值
 * \param delta 变化量
 * \param range 被操作的值变化范围，正数时起效
 */
inline void CircleAdd(float *origin, float delta, float range) {
  float out = *origin + delta;
  if (range > 0.0f) {
    if (out >= range)
      out -= range;
    else if (out < 0.0f)
      out += range;
  }
  *origin = out;
}
