/*
  二阶曲线拟合。
*/

#pragma once

#include "component.hpp"

namespace Component {
class SecOrderFunction {
 public:
  /* y=ax^2+bx+c */
  typedef struct {
    float a;
    float b;
    float c;
    float max;
    float min;
  } Param;

  SecOrderFunction(Param& param);

  float GetValue(float fb);

  Param param_;
};
}  // namespace Component
