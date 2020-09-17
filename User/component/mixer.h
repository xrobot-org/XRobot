/*
  混合器
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "user_math.h"

typedef enum {
  MIXER_MECANUM, /* 混合器模式 */
  MIXER_PARLFIX4, /* 混合器模式 */
  MIXER_PARLFIX2, /* 混合器模式 */
  MIXER_OMNICROSS, /* 混合器模式 */
  MIXER_OMNIPLUS, /* 混合器模式 */
} Mixer_Mode_t; /* 混合器模式 */

typedef struct {
  Mixer_Mode_t mode;
} Mixer_t;  /* 混合器主结构体 */

int8_t Mixer_Init(Mixer_t *mixer, Mixer_Mode_t mode);
int8_t Mixer_Apply(Mixer_t *mixer, float vx, float vy, float wz, float *out,
                   int8_t len);

#ifdef __cplusplus
}
#endif
