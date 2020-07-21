/* 
	限制器
*/

#include "limiter.h"

int8_t HeatLimiter_Apply(float heat_limit, float vbat, float dt_sec) {
	(void)heat_limit;
	(void)vbat;
	(void)dt_sec;
	return 0;
}

int8_t PowerLimit_Apply(float power_limit, float vbat, float *motor, uint32_t len) {
	if (motor == NULL)
		return -1;
	
	float total_current = 0.f;
	for(uint32_t i = 0; i < len; i++) {
		total_current += fabsf(*motor);
	}
	
	if (power_limit > 0.f) {
		if ((total_current * vbat) > power_limit) {
			float current_scale = total_current / power_limit / vbat;
			for(uint32_t i = 0; i < len; i++) {
				*motor *= current_scale;
			}
		}
	}
	return 0;
}
