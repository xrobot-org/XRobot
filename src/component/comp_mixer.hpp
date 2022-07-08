/*
  混合器
*/

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "comp_type.hpp"

/** 四轮布局 */
/* 前 */
/* 2 1 */
/* 3 4 */

/* 两轮布局 */
/* 前 */
/* 2 1 */

namespace Component {
class Mixer {
 public:
  typedef enum {
    /* 底面用 */
    MECANUM,   /* 麦克纳姆轮 */
    PARLFIX4,  /* 平行四驱动轮 */
    PARLFIX2,  /* 平行对侧两驱动轮 */
    OMNICROSS, /* 叉形全向轮 */
    OMNIPLUS,  /* 十字全向轮 */
    SINGLE,    /* 单个摩擦轮 */
    NONE,      /* 不可移动底盘 */
  } Mode;

  Mixer(Mode mode);

  bool Apply(Component::Type::MoveVector &move_vec, float *out);

  Mode mode_;

  uint8_t len_;
};
}  // namespace Component
