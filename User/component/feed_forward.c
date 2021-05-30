/**
 * @file feed_forward.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 前馈控制
 * @version 0.1
 * @date 2021-05-30
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "feed_forward.h"

/**
 * @brief 初始化FeedForward
 *
 * @param ff FeedForward结构体
 * @param sample_freq 采样频率
 * @param param FeedForward参数
 */
void FeedForward_Init(FeedForward_t *ff, float sample_freq,
                      const FeedForward_Params_t *param) {
  ASSERT(ff);
  ASSERT(param);

  ASSERT(isfinite(param->gain));
  ff->param = param;

  float dt_min = 1.0f / sample_freq;
  ASSERT(isfinite(dt_min));
  ff->dt_min = dt_min;

  FeedForward_Reset(ff);
}

/**
 * @brief FeedForward计算
 *
 * @param ff FeedForward结构体
 * @param sp 设定值
 * @param dt 间隔时间
 * @return float 计算的输出
 */
float FeedForward_Calc(FeedForward_t *ff, float sp, float dt) {
  ASSERT(ff);

  if (!isfinite(sp) || !isfinite(dt)) {
    return ff->last.out;
  }
  const float delta_sp = sp - ff->last.sp;
  const float out = delta_sp / dt * ff->param->gain;
  ff->last.sp = sp;
  ff->last.out = out;
  return out;
}

/**
 * @brief 重置FeedForward
 *
 * @param ff FeedForward结构体
 */
void FeedForward_Reset(FeedForward_t *ff) {
  ASSERT(ff);

  ff->last.sp = 0.0f;
  ff->last.out = 0.0f;
}
