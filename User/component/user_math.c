/*
        自定义的数学运算。

*/

#include "user_math.h"

float InvSqrt(float x) {
#if 0
	float out;
	arm_sqrt_f32(x, &out);
	return 1.f/out;
#else
  return 1.f / sqrtf(x);
#endif
}

float AbsClip(float in, float limit) {
  return (in < -limit) ? -limit : ((in > limit) ? limit : in);
}

float Sign(float in) { return (in > 0) ? 1.f : 0.f; }
