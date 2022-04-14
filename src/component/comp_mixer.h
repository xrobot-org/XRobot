/*
  混合器
*/

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "comp_type.h"

/** 四轮布局 */
/* 前 */
/* 2 1 */
/* 3 4 */

/* 两轮布局 */
/* 前 */
/* 2 1 */

/* 混合器模式 */
typedef enum {
  /* 底面用 */
  MIXER_MECANUM,   /* 麦克纳姆轮 */
  MIXER_PARLFIX4,  /* 平行四驱动轮 */
  MIXER_PARLFIX2,  /* 平行对侧两驱动轮 */
  MIXER_OMNICROSS, /* 叉形全向轮 */
  MIXER_OMNIPLUS,  /* 十字全向轮 */
  MIXER_SINGLE,    /* 单个摩擦轮 */
  MIXER_NONE,      /* 不可移动底盘 */

  /* 空中用 */
  MIXER_CANARD, /* 飞镖鸭翼 */
} mixer_mode_t;

typedef struct {
  mixer_mode_t mode;
} mixer_t; /* 混合器主结构体 */

/**
 * @brief 初始化混合器
 *
 * @param mixer 混合器
 * @param mode 混合器模式
 * @return int8_t 0对应没有错误
 */
int8_t mixer_init(mixer_t *mixer, mixer_mode_t mode);

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
int8_t mixer_apply(mixer_t *mixer, move_vector_t *move_vec, float *out,
                   size_t len, float scale_factor);
