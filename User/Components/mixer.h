/* 
	混合器
*/


#pragma once

#include <stdint.h>
#include <stdbool.h>

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

int Mixer_Init(Mixer_t *mixer, Mixer_Mode_t mode);
int Mixer_Apply(Mixer_t *mixer, float vx, float vy, float wz, float *out, int len);
