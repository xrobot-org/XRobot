/* 
	自定义的数学运算。

*/


#include "user_math.h"


float InvSqrt(float x) {
	return 1.f/sqrtf(x);
}

float AbsClip(float in, float limit) {
	return (in < -limit) ? -limit : ((in > limit) ? limit : in);
}

float Sign(float in) {
	return (in > 0) ? 1.f : 0.f;
}
