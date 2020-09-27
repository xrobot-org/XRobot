/*
  自定义的数学运算。
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx.h"
#define ARM_MATH_CM4

#include <float.h>
#include <math.h>

#define MATH_DEG_TO_RAD_MULT (0.01745329251f)
#define MATH_RAD_TO_DEG_MULT (57.2957795131f)

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

typedef struct {
  float vx;     /* 前后平移 */
  float vy;     /* 左右平移 */
  float wz;     /* 转动 */
} MoveVector_t; /* 移动向量 */

float InvSqrt(float x);

float AbsClip(float in, float limit);

float Sign(float in);

void ResetMoveVector(MoveVector_t *mv);

float CalcCircleError(float sp, float fb, float range);

#ifdef __cplusplus
}
#endif
