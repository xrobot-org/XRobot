/*
  各类滤波器。
*/

#pragma once

#include "comp_utils.hpp"

/* 二阶低通滤波器 */
typedef struct {
  float cutoff_freq; /* 截止频率 */

  float a1;
  float a2;

  float b0;
  float b1;
  float b2;

  float delay_element_1;
  float delay_element_2;

} low_pass_filter_2p_t;

/* 带阻滤波器 */
typedef struct {
  float notch_freq; /* 阻止频率 */
  float bandwidth;  /* 带宽 */

  float a1;
  float a2;

  float b0;
  float b1;
  float b2;
  float delay_element_1;
  float delay_element_2;

} notch_filter_t;

/**
 * @brief 初始化滤波器
 *
 * @param f 滤波器
 * @param sample_freq 采样频率
 * @param cutoff_freq 截止频率
 */
void low_pass_filter_2p_init(low_pass_filter_2p_t *f, float sample_freq,
                             float cutoff_freq);

/**
 * @brief 施加一次滤波计算
 *
 * @param f 滤波器
 * @param sample 采样的值
 * @return float 滤波后的值
 */
float low_pass_filter_2p_apply(low_pass_filter_2p_t *f, float sample);

/**
 * @brief 重置滤波器
 *
 * @param f 滤波器
 * @param sample 采样的值
 * @return float 滤波后的值
 */
float low_pass_filter_2p_reset(low_pass_filter_2p_t *f, float sample);

/**
 * @brief 初始化滤波器
 *
 * @param f 滤波器
 * @param sample_freq 采样频率
 * @param notch_freq 中心频率
 * @param bandwidth 带宽
 */
void notch_filter_init(notch_filter_t *f, float sample_freq, float notch_freq,
                       float bandwidth);

/**
 * @brief 施加一次滤波计算
 *
 * @param f 滤波器
 * @param sample 采样的值
 * @return float 滤波后的值
 */
float notch_filter_apply(notch_filter_t *f, float sample);

/**
 * @brief 重置滤波器
 *
 * @param f 滤波器
 * @param sample 采样的值
 * @return float 滤波后的值
 */
float notch_filter_reset(notch_filter_t *f, float sample);
