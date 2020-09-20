/*
  混合器
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "user_math.h"

typedef enum {
  MIXER_MECANUM,   /* 麦克纳姆轮 */
  MIXER_PARLFIX4,  /* 平行四驱动轮 */
  MIXER_PARLFIX2,  /* 平行对侧两驱动轮 */
  MIXER_OMNICROSS, /* 叉形全向轮 */
  MIXER_OMNIPLUS,  /* 十字全向轮 */
} Mixer_Mode_t;    /* 混合器模式 */

typedef struct {
  Mixer_Mode_t mode;
} Mixer_t; /* 混合器主结构体 */

int8_t Mixer_Init(Mixer_t *mixer, Mixer_Mode_t mode);
int8_t Mixer_Apply(Mixer_t *mixer, float vx, float vy, float wz, float *out,
                   int8_t len, float scale);

#ifdef __cplusplus
}
#endif
