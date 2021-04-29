/*
  混合器
*/

#include "mixer.h"

/**
 * @brief 初始化混合器
 *
 * @param mixer 混合器
 * @param mode 混合器模式
 * @return int8_t 0对应没有错误
 */
int8_t Mixer_Init(Mixer_t *mixer, Mixer_Mode_t mode) {
  ASSERT(mixer);

  mixer->mode = mode;
  return 0;
}

/**
 * @brief 计算输出
 *
 * @param mixer 混合器
 * @param move_vec 运动向量
 * @param out 输出数组
 * @param len 输出数组长短
 * @param scale_factor 输出放大因子
 * @return int8_t 0对应没有错误
 */
int8_t Mixer_Apply(Mixer_t *mixer, MoveVector_t *move_vec, float *out,
                   size_t len, float scale_factor) {
  ASSERT(mixer);
  ASSERT(move_vec);
  ASSERT(out);

  switch (mixer->mode) {
    case MIXER_MECANUM:
      ASSERT(len == 4);
      out[0] = move_vec->vx - move_vec->vy + move_vec->wz;
      out[1] = move_vec->vx + move_vec->vy + move_vec->wz;
      out[2] = -move_vec->vx + move_vec->vy + move_vec->wz;
      out[3] = -move_vec->vx - move_vec->vy + move_vec->wz;
      break;

    case MIXER_PARLFIX4:
      ASSERT(len == 4);
      out[0] = -move_vec->vx;
      out[1] = move_vec->vx;
      out[2] = move_vec->vx;
      out[3] = -move_vec->vx;
      break;

    case MIXER_PARLFIX2:
      ASSERT(len == 2);
      out[0] = -move_vec->vx;
      out[1] = move_vec->vx;
      break;

    case MIXER_SINGLE:
      ASSERT(len == 1);
      out[0] = move_vec->vx;
      break;

    case MIXER_OMNICROSS:
    case MIXER_OMNIPLUS:
      break;

    default:
      break;
  }

  float abs_max = 0.f;
  for (size_t i = 0; i < len; i++) {
    const float abs_val = fabsf(out[i]);
    abs_max = (abs_val > abs_max) ? abs_val : abs_max;
  }
  if (abs_max > 1.f) {
    for (size_t i = 0; i < len; i++) {
      out[i] /= abs_max;
    }
  }
  for (size_t i = 0; i < len; i++) {
    out[i] *= scale_factor;
  }
  return 0;
}
