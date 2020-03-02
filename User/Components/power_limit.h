/* 
	Modified from https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.h

*/

#pragma once

#include <stdint.h>

int PowerLimit_Apply(float power_limit, float vbat, float *motor, uint32_t len);
