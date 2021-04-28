/*
  各类滤波器。
*/

#include "filter.h"

#include "user_math.h"

/**
 * @brief 初始化滤波器
 *
 * @param f 滤波器
 * @param sample_freq 采样频率
 * @param cutoff_freq 截止频率
 */
void LowPassFilter2p_Init(LowPassFilter2p_t *f, float sample_freq,
                          float cutoff_freq) {
  ASSERT(f);

  f->cutoff_freq = cutoff_freq;

  f->delay_element_1 = 0.0f;
  f->delay_element_2 = 0.0f;

  if (f->cutoff_freq <= 0.0f) {
    /* no filtering */
    f->b0 = 1.0f;
    f->b1 = 0.0f;
    f->b2 = 0.0f;

    f->a1 = 0.0f;
    f->a2 = 0.0f;

    return;
  }
  const float fr = sample_freq / f->cutoff_freq;
  const float ohm = tanf(M_PI / fr);
  const float c = 1.0f + 2.0f * cosf(M_PI / 4.0f) * ohm + ohm * ohm;

  f->b0 = ohm * ohm / c;
  f->b1 = 2.0f * f->b0;
  f->b2 = f->b0;

  f->a1 = 2.0f * (ohm * ohm - 1.0f) / c;
  f->a2 = (1.0f - 2.0f * cosf(M_PI / 4.0f) * ohm + ohm * ohm) / c;
}

/**
 * @brief 施加一次滤波计算
 *
 * @param f 滤波器
 * @param sample 采样的值
 * @return float 滤波后的值
 */
float LowPassFilter2p_Apply(LowPassFilter2p_t *f, float sample) {
  ASSERT(f);

  /* do the filtering */
  float delay_element_0 =
      sample - f->delay_element_1 * f->a1 - f->delay_element_2 * f->a2;

  if (isinf(delay_element_0)) {
    /* don't allow bad values to propagate via the filter */
    delay_element_0 = sample;
  }

  const float output = delay_element_0 * f->b0 + f->delay_element_1 * f->b1 +
                       f->delay_element_2 * f->b2;

  f->delay_element_2 = f->delay_element_1;
  f->delay_element_1 = delay_element_0;

  /* return the value. Should be no need to check limits */
  return output;
}

/**
 * @brief 重置滤波器
 *
 * @param f 滤波器
 * @param sample 采样的值
 * @return float 滤波后的值
 */
float LowPassFilter2p_Reset(LowPassFilter2p_t *f, float sample) {
  ASSERT(f);

  const float dval = sample / (f->b0 + f->b1 + f->b2);

  if (isfinite(dval)) {
    f->delay_element_1 = dval;
    f->delay_element_2 = dval;

  } else {
    f->delay_element_1 = sample;
    f->delay_element_2 = sample;
  }

  return LowPassFilter2p_Apply(f, sample);
}

/**
 * @brief 初始化滤波器
 *
 * @param f 滤波器
 * @param sample_freq 采样频率
 * @param notch_freq 中心频率
 * @param bandwidth 带宽
 */
void NotchFilter_Init(NotchFilter_t *f, float sample_freq, float notch_freq,
                      float bandwidth) {
  ASSERT(f);

  f->notch_freq = notch_freq;
  f->bandwidth = bandwidth;

  f->delay_element_1 = 0.0f;
  f->delay_element_2 = 0.0f;

  if (notch_freq <= 0.0f) {
    /* no filtering */
    f->b0 = 1.0f;
    f->b1 = 0.0f;
    f->b2 = 0.0f;

    f->a1 = 0.0f;
    f->a2 = 0.0f;

    return;
  }

  const float alpha = tanf(M_PI * bandwidth / sample_freq);
  const float beta = -cosf(M_2PI * notch_freq / sample_freq);
  const float a0_inv = 1.0f / (alpha + 1.0f);

  f->b0 = a0_inv;
  f->b1 = 2.0f * beta * a0_inv;
  f->b2 = a0_inv;

  f->a1 = f->b1;
  f->a2 = (1.0f - alpha) * a0_inv;
}

/**
 * @brief 施加一次滤波计算
 *
 * @param f 滤波器
 * @param sample 采样的值
 * @return float 滤波后的值
 */
inline float NotchFilter_Apply(NotchFilter_t *f, float sample) {
  ASSERT(f);

  /* Direct Form II implementation */
  const float delay_element_0 =
      sample - f->delay_element_1 * f->a1 - f->delay_element_2 * f->a2;
  const float output = delay_element_0 * f->b0 + f->delay_element_1 * f->b1 +
                       f->delay_element_2 * f->b2;

  f->delay_element_2 = f->delay_element_1;
  f->delay_element_1 = delay_element_0;

  return output;
}

/**
 * @brief 重置滤波器
 *
 * @param f 滤波器
 * @param sample 采样的值
 * @return float 滤波后的值
 */
float NotchFilter_Reset(NotchFilter_t *f, float sample) {
  ASSERT(f);

  float dval = sample;

  if (fabsf(f->b0 + f->b1 + f->b2) > FLT_EPSILON) {
    dval = dval / (f->b0 + f->b1 + f->b2);
  }

  f->delay_element_1 = dval;
  f->delay_element_2 = dval;

  return NotchFilter_Apply(f, sample);
}
