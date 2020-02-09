/* 
	底盘混合器。

*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

int Mixer_Mecanum(float vx, float vy, float wz, float *out, int len);
int Mixer_ParlFix4(float vx, float vy, float wz, float *out, int len);
int Mixer_ParlFix2(float vx, float vy, float wz, float *out, int len);
int Mixer_OmniCross(float vx, float vy, float wz, float *out, int len);
int Mixer_OmniPlus(float vx, float vy, float wz, float *out, int len);
