/**
 * @file feed_forward.h
 * @author Qu Shen (503578404@qq.com)
 * @brief 前馈控制
 * @version 0.1
 * @date 2021-05-30
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#include "utils.h"

/* FeedForward参数 */
typedef struct {
  float gain; /* 增益 */
} FeedForward_Params_t;

/* FeedForward主结构体 */
typedef struct {
  const FeedForward_Params_t *param;
  float dt_min; /* 最小Calc调用间隔 */

  struct {
    float sp;
    float out;
  } last;
} FeedForward_t;

/**
 * @brief 初始化FeedForward
 *
 * @param ff FeedForward结构体
 * @param sample_freq 采样频率
 * @param param FeedForward参数s
 */
void FeedForward_Init(FeedForward_t *ff, float sample_freq,
                      const FeedForward_Params_t *param);

/**
 * @brief FeedForward计算
 *
 * @param ff FeedForward结构体
 * @param sp 设定值
 * @param dt 间隔时间
 */
float FeedForward_Calc(FeedForward_t *ff, float sp, float dt);

/**
 * @brief 重置FeedForward
 *
 * @param ff FeedForward结构体
 */
void FeedForward_Reset(FeedForward_t *ff);
