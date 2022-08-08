/*
  各类滤波器。
*/

#include "comp_filter.hpp"

#include "comp_utils.hpp"

using namespace Component;

LowPassFilter::LowPassFilter(float cut_freq)
    : cut_freq_(cut_freq), last_out_(0) {}

float LowPassFilter::Apply(float sample, float dt) {
  float k = 2 * M_2PI * this->cut_freq_ * dt;
  k = k / (1 + k);
  float out = k * sample + (1 - k) * this->last_out_;
  this->last_out_ = out;

  return out;
}

void LowPassFilter::Reset(float sample) { this->last_out_ = sample; }

LowPassFilter2p::LowPassFilter2p(float sample_freq, float cutoff_freq) {
  this->cutoff_freq_ = cutoff_freq;

  this->delay_element_1_ = 0.0f;
  this->delay_element_2_ = 0.0f;

  if (this->cutoff_freq_ <= 0.0f) {
    /* no filtering */
    this->b0_ = 1.0f;
    this->b1_ = 0.0f;
    this->b2_ = 0.0f;

    this->a1_ = 0.0f;
    this->a2_ = 0.0f;

    return;
  }
  const float fr = sample_freq / this->cutoff_freq_;
  const float ohm = tanf(M_PI / fr);
  const float c = 1.0f + 2.0f * cosf(M_PI / 4.0f) * ohm + ohm * ohm;

  this->b0_ = ohm * ohm / c;
  this->b1_ = 2.0f * this->b0_;
  this->b2_ = this->b0_;

  this->a1_ = 2.0f * (ohm * ohm - 1.0f) / c;
  this->a2_ = (1.0f - 2.0f * cosf(M_PI / 4.0f) * ohm + ohm * ohm) / c;
}

float LowPassFilter2p::Apply(float sample) {
  /* do the filtering */
  float delay_element_0 = sample - this->delay_element_1_ * this->a1_ -
                          this->delay_element_2_ * this->a2_;

  if (isinf(delay_element_0)) {
    /* don't allow bad values to propagate via the filter */
    delay_element_0 = sample;
  }

  const float output = delay_element_0 * this->b0_ +
                       this->delay_element_1_ * this->b1_ +
                       this->delay_element_2_ * this->b2_;

  this->delay_element_2_ = this->delay_element_1_;
  this->delay_element_1_ = delay_element_0;

  /* return the value. Should be no need to check limits */
  return output;
}

float LowPassFilter2p::Reset(float sample) {
  const float dval = sample / (this->b0_ + this->b1_ + this->b2_);

  if (isfinite(dval)) {
    this->delay_element_1_ = dval;
    this->delay_element_2_ = dval;

  } else {
    this->delay_element_1_ = sample;
    this->delay_element_2_ = sample;
  }

  return this->Apply(sample);
}
