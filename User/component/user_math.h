/* 
	自定义的数学运算。

*/


#pragma once

#include "stm32f4xx.h"
#define ARM_MATH_CM4

#include <math.h>
#include <float.h>
#include <stdbool.h>
#include <string.h>

#define MATH_DEG_TO_RAD_MULT	(0.01745329251f)
#define MATH_RAD_TO_DEG_MULT	(57.2957795131f)

#ifndef M_PI
    #define M_PI 3.14159265358979323846f
#endif

typedef struct {
	float vx;
	float vy;
	float wz;
} MoveVector_t;

float InvSqrt(float x);

float AbsClip(float in, float limit);

float Sign(float in);
