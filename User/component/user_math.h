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

#define M_DEG2RAD_MULT (0.01745329251f)
#define M_RAD2DEG_MULT (57.2957795131f)

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#ifndef M_2PI
#define M_2PI 6.28318530717958647692f
#endif

typedef struct {
  float vx;     /* 前后平移 */
  float vy;     /* 左右平移 */
  float wz;     /* 转动 */
} MoveVector_t; /* 移动向量 */

float InvSqrt(float x);

float AbsClip(float in, float limit);

float Sign(float in);

/*!
 * \brief 将运动向量置零
 * 
 * \param mv 被操作的值
 */
void ResetMoveVector(MoveVector_t *mv);

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
float CircleError(float sp, float fb, float range);

/*!
 * \brief 循环加法，用于没有负数值，并在一定范围内变化的值
 * 例如编码器，在0-2PI内变化，1.5PI + 1.5PI = 1PI
 *
 * \param origin 被操作的值
 * \param delta 变化量
 * \param range 被操作的值变化范围，正数时起效
 */
void CircleAdd(float *origin, float delta, float range);

#ifdef __cplusplus
}
#endif
