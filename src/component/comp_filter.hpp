/*
  各类滤波器。
*/

#pragma once

#include "comp_utils.hpp"

namespace Component {
/* 一阶数字低通滤波器 */
class LowPassFilter {
 public:
  LowPassFilter(float k);

  float Apply(float sample);

  void Reset(float sample);

  float k_;

  float last_out_;
};
/* 二阶巴特沃斯低通滤波器 */
class LowPassFilter2p {
 public:
  float cutoff_freq_; /* 截止频率 */

  float a1_;
  float a2_;

  float b0_;
  float b1_;
  float b2_;

  float delay_element_1_;
  float delay_element_2_;

  LowPassFilter2p(float sample_freq, float cutoff_freq);

  float Apply(float sample);

  float Reset(float sample);
};
}  // namespace Component
