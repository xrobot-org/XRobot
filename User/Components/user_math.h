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

typedef struct {
	float32_t vx;
	float32_t vy;
	float32_t wz;
} MoveVector_t;

float32_t InvSqrt(float32_t x);

float32_t AbsClip(float32_t in, float32_t limit);

float32_t Sign(float32_t in);
