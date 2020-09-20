/*
  混合器
*/

#include "mixer.h"

int8_t Mixer_Init(Mixer_t *mixer, Mixer_Mode_t mode) {
  if (mixer == NULL) return -1;

  mixer->mode = mode;
  return 0;
}

int8_t Mixer_Apply(Mixer_t *mixer, float vx, float vy, float wz, float *out,
                   int8_t len, float scale) {
  if (mixer == NULL) return -1;

  switch (mixer->mode) {
    case MIXER_MECANUM:
      if (len == 4) {
        out[0] = -vx - vy + wz;
        out[1] = vx - vy + wz;
        out[2] = vx + vy + wz;
        out[3] = -vx + vy + wz;
      } else {
        goto error;
      }
      break;

    case MIXER_PARLFIX4:
      if (len == 4) {
        out[0] = -vx;
        out[1] = vx;
        out[2] = vx;
        out[3] = -vx;
      } else {
        goto error;
      }
    case MIXER_PARLFIX2:
      if (len == 2) {
        out[0] = -vx;
        out[1] = vx;
      } else {
        goto error;
      }
    case MIXER_OMNICROSS:
    case MIXER_OMNIPLUS:
      goto error;
  }

  for (int8_t i = 0; i < len; i++) {
    out[i] *= scale;
  }
  return 0;

error:
  for (uint8_t i = 0; i < len; i++) out[i] = 0;
  return -1;
}
