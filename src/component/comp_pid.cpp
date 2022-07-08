/*
   Modified from
   https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.cpp

   参考资料：
   https://github.com/PX4/Firmware/issues/12362
   https://dev.px4.io/master/en/flight_stack/controller_diagrams.html
   https://docs.px4.io/master/en/config_mc/pid_tuning_guide_multicopter.html#standard_form
   https://www.controleng.com/articles/not-all-pid-controllers-are-the-same/
   https://en.wikipedia.org/wiki/PID_controller
   http://brettbeauregard.com/blog/2011/04/improving-the-beginner%E2%80%99s-pid-derivative-kick/
*/

#include "comp_pid.hpp"

#define SIGMA 0.000001f

using namespace Component;

PID::PID(PID::Param &param, float sample_freq)
    : param_(param), dfilter_(sample_freq, param.d_cutoff_freq) {
  float dt_min = 1.0f / sample_freq;
  ASSERT(isfinite(dt_min));
  this->dt_min_ = dt_min;

  this->Reset();
}

float PID::Calculate(float sp, float fb, float dt) {
  if (!isfinite(sp) || !isfinite(fb) || !isfinite(dt)) {
    return this->last.out_;
  }

  /* 计算误差值 */
  const float err = circle_error(sp, fb, this->param_.range);

  /* 计算P项 */
  float k_err = err * this->param_.k;

  /* 计算D项 */
  const float k_fb = this->param_.k * fb;
  const float filtered_k_fb = this->dfilter_.Apply(k_fb);

  /* 通过fb计算D，避免了由于sp变化导致err突变的问题 */
  /* 当sp不变时，err的微分等于负的fb的微分 */
  float d = (filtered_k_fb - this->last.k_fb_) / fmaxf(dt, this->dt_min_);

  this->last.err_ = err;
  this->last.k_fb_ = filtered_k_fb;

  if (!isfinite(d)) d = 0.0f;

  /* 计算PD输出 */
  float output = (k_err * this->param_.p) - (d * this->param_.d);

  /* 计算I项 */
  const float i = this->i_ + (k_err * dt);
  const float i_out = i * this->param_.i;

  if (this->param_.i > SIGMA) {
    /* 检查是否饱和 */
    if (isfinite(i)) {
      if ((fabsf(output + i_out) <= this->param_.out_limit) &&
          (fabsf(i) <= this->param_.i_limit)) {
        /* 未饱和，使用新积分 */
        this->i_ = i;
      }
    }
  }

  /* 计算PID输出 */
  output += i_out;

  /* 限制输出 */
  if (isfinite(output)) {
    if (this->param_.out_limit > SIGMA) {
      output = abs_clampf(output, this->param_.out_limit);
    }
    this->last.out_ = output;
  }
  return this->last.out_;
}
float PID::Calculate(float sp, float fb, float fb_dot, float dt) {
  if (!isfinite(sp) || !isfinite(fb) || !isfinite(fb_dot) || !isfinite(dt)) {
    return this->last.out_;
  }

  /* 计算误差值 */
  const float err = circle_error(sp, fb, this->param_.range);

  /* 计算P项 */
  float k_err = err * this->param_.k;

  /* 计算D项 */
  const float k_fb = this->param_.k * fb;
  const float filtered_k_fb = this->dfilter_.Apply(k_fb);

  float d;

  d = fb_dot;

  this->last.err_ = err;
  this->last.k_fb_ = filtered_k_fb;

  if (!isfinite(d)) d = 0.0f;

  /* 计算PD输出 */
  float output = (k_err * this->param_.p) - (d * this->param_.d);

  /* 计算I项 */
  const float i = this->i_ + (k_err * dt);
  const float i_out = i * this->param_.i;

  if (this->param_.i > SIGMA) {
    /* 检查是否饱和 */
    if (isfinite(i)) {
      if ((fabsf(output + i_out) <= this->param_.out_limit) &&
          (fabsf(i) <= this->param_.i_limit)) {
        /* 未饱和，使用新积分 */
        this->i_ = i;
      }
    }
  }

  /* 计算PID输出 */
  output += i_out;

  /* 限制输出 */
  if (isfinite(output)) {
    if (this->param_.out_limit > SIGMA) {
      output = abs_clampf(output, this->param_.out_limit);
    }
    this->last.out_ = output;
  }
  return this->last.out_;
}
void PID::SetK(float k) { this->param_.k = k; };

void PID::Reset() {
  this->i_ = 0.0f;
  this->last.err_ = 0.0f;
  this->last.k_fb_ = 0.0f;
  this->last.out_ = 0.0f;
  this->dfilter_.Reset(0.0f);
}
