/* 
	混合器
*/


#pragma once

#include <stdint.h>
#include <stdbool.h>
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
int8_t Mixer_Apply(Mixer_t *mixer, float32_t vx, float32_t vy, float32_t wz, float32_t *out, int8_t len);
