/* 
	自定义的数学运算。

*/

#include "user_math.h"


float32_t InvSqrt(float32_t x) {
#if 0
	float32_t out;
	arm_sqrt_f32(x, &out);
	return 1.f/out;
#else
	return 1.f/sqrtf(x);
#endif
}

float32_t AbsClip(float32_t in, float32_t limit) {
	return (in < -limit) ? -limit : ((in > limit) ? limit : in);
}

float32_t Sign(float32_t in) {
	return (in > 0) ? 1.f : 0.f;
}
