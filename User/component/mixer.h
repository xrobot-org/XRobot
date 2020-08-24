/* 
	混合器
*/


#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

#include "user_math.h"

typedef enum {
	MIXER_MECANUM,
	MIXER_PARLFIX4,
	MIXER_PARLFIX2,
	MIXER_OMNICROSS,
	MIXER_OMNIPLUS,
} Mixer_Mode_t;

typedef struct {
	Mixer_Mode_t mode;
} Mixer_t;

int8_t Mixer_Init(Mixer_t *mixer, Mixer_Mode_t mode);
int8_t Mixer_Apply(Mixer_t *mixer, float vx, float vy, float wz, float *out, int8_t len);

#ifdef __cplusplus
}
#endif
