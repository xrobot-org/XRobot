/*
  Modified from
  https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.h
*/

#pragma once

#include "comp_filter.hpp"
#include "component.hpp"

namespace Component {
class PID {
 public:
  /* PID参数 */
  typedef struct {
    float k;             /* 控制器增益，设置为1用于并行模式 */
    float p;             /* 比例项增益，设置为1用于标准形式 */
    float i;             /* 积分项增益 */
    float d;             /* 微分项增益 */
    float i_limit;       /* 积分项上限 */
    float out_limit;     /* 输出绝对值限制 */
    float d_cutoff_freq; /* D项低通截止频率 */
    float range;         /* 计算循环误差时使用，大于0时启用 */
  } Param;

  PID(Param &param, float sample_freq);

  void SetK(float k);

  void Reset();

  float Calculate(float sp, float fb, float dt);

  float Calculate(float sp, float fb, float fb_dot, float dt);

  Param param_;

  float dt_min_; /* 最小PID_Calc调用间隔 */
  float i_;      /* 积分 */

  struct {
    float err_;  /* 上次误差 */
    float k_fb_; /* 上次反馈值 */
    float out_;  /* 上次输出 */
  } last;

  LowPassFilter2p dfilter_;
};
}  // namespace Component
