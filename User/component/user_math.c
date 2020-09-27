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

inline void ResetMoveVector(MoveVector_t *mv) {
  memset(mv, 0, sizeof(MoveVector_t));
}

inline float CalcCircleError(float sp, float fb, float range) {
  float error = fb - sp;
  float half_range = range / 2;
  if (error > half_range) {
    error -= half_range;
    return -error;
  }
  return error;
}
