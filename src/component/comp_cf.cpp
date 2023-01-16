/*
  二阶曲线拟合。
*/

#include "comp_cf.hpp"

using namespace Component;

SecOrderFunction::SecOrderFunction(SecOrderFunction::Param& param)
    : param_(param) {}

float SecOrderFunction::GetValue(float fb) {
  float out =
      this->param_.a * powf(fb, 2) + this->param_.b * fb + this->param_.c;
  clampf(&out, this->param_.min, this->param_.max);
  return out;
}
