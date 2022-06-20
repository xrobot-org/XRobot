/*
  二阶曲线拟合。
*/

#pragma once

#include "comp_utils.hpp"

/* y=ax^2+bx+c */
typedef struct {
  float a;
  float b;
  float c;
  float max;
  float min;
} cf_param_t;

float cf_get_value(const cf_param_t* param, float fb);
