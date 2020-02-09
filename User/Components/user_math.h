/* 
	自定义的数学运算。

*/


#pragma once

#include "stm32f4xx.h"

#include <arm_math.h>
#include <float.h>


float InvSqrt(float x);

float AbsClip(float in, float limit);

float Sign(float in);
