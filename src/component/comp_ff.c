/*
  前馈控制算法。

  使用二阶函数拟合输出值，抵消重力
*/

#include "comp_ff.h"

float ff_get_value(const ff_param_t* param, float fb) {
  float out = param->a * pow(fb, 2) + param->b * fb + param->c;
  clampf(&out, param->min, param->max);
  return out;
}
