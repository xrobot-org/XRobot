/* 
	底盘混合器。

*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef int Mixer(float vx, float vy, float vz, float *lf, float *lr, float *rf, float *rr);

int Mixer_Mecanum(float vx, float vy, float vz, float *lf, float *lr, float *rf, float *rr);
int Mixer_ParlFix4(float vx, float vy, float vz, float *lf, float *lr, float *rf, float *rr);
int Mixer_OmniCross(float vx, float vy, float vz, float *lf, float *lr, float *rf, float *rr);
int Mixer_OmniPlus(float vx, float vy, float vz, float *lf, float *lr, float *rf, float *rr);
