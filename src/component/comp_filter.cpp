/*
  各类滤波器。
*/

#include "comp_filter.hpp"

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

LowPassFilter2p::LowPassFilter2p(float sample_freq, float cutoff_freq)
    : cutoff_freq_(cutoff_freq),
      delay_element_1_(0.0f),
      delay_element_2_(0.0f) {
  if (this->cutoff_freq_ <= 0.0f) {
    /* no filtering */
    this->b0_ = 1.0f;
    this->b1_ = 0.0f;
    this->b2_ = 0.0f;

    this->a1_ = 0.0f;
    this->a2_ = 0.0f;

    return;
  }
  const float FR = sample_freq / this->cutoff_freq_;
  const float OHM = tanf(M_PI / FR);
  const float C = 1.0f + 2.0f * cosf(M_PI / 4.0f) * OHM + OHM * OHM;

  this->b0_ = OHM * OHM / C;
  this->b1_ = 2.0f * this->b0_;
  this->b2_ = this->b0_;

  this->a1_ = 2.0f * (OHM * OHM - 1.0f) / C;
  this->a2_ = (1.0f - 2.0f * cosf(M_PI / 4.0f) * OHM + OHM * OHM) / C;
}

float LowPassFilter2p::Apply(float sample) {
  /* do the filtering */
  float delay_element_0 = sample - this->delay_element_1_ * this->a1_ -
                          this->delay_element_2_ * this->a2_;

  if (isinf(delay_element_0)) {
    /* don't allow bad values to propagate via the filter */
    delay_element_0 = sample;
  }

  const float OUTPUT = delay_element_0 * this->b0_ +
                       this->delay_element_1_ * this->b1_ +
                       this->delay_element_2_ * this->b2_;

  this->delay_element_2_ = this->delay_element_1_;
  this->delay_element_1_ = delay_element_0;

  /* return the value. Should be no need to check limits */
  return OUTPUT;
}

float LowPassFilter2p::Reset(float sample) {
  const float DVAL = sample / (this->b0_ + this->b1_ + this->b2_);

  if (isfinite(DVAL)) {
    this->delay_element_1_ = DVAL;
    this->delay_element_2_ = DVAL;

  } else {
    this->delay_element_1_ = sample;
    this->delay_element_2_ = sample;
  }

  return this->Apply(sample);
}
