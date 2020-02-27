/* 
	自定义的数学运算。

*/


#pragma once

#include "stm32f4xx.h"

#define ARM_MATH_CM4
#include <arm_math.h>

#include <float.h>

#define MATH_DEGREE_TO_RADIAN_MULTIPLIER	(0.01745329251f)
#define MATH_RADIAN_TO_DEGREE_MULTIPLIER	(57.2957795131f)

float InvSqrt(float x);

float AbsClip(float in, float limit);

float Sign(float in);
