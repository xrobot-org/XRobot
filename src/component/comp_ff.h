/*
  前馈控制算法。

  使用二阶函数拟合输出值，抵消重力
*/

#pragma once

#include "comp_utils.h"

/* y=ax^2+bx+c */
typedef struct {
  float a;
  float b;
  float c;
  float max;
  float min;
} ff_param_t;

float ff_get_value(const ff_param_t* param, float fb);
