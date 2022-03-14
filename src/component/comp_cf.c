/*
  二阶曲线拟合。
*/

#include "comp_cf.h"

float cf_get_value(const cf_param_t* param, float fb) {
  float out = param->a * pow(fb, 2) + param->b * fb + param->c;
  clampf(&out, param->min, param->max);
  return out;
}
