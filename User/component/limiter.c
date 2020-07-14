/* 
	限制器
*/

#include "limiter.h"

int8_t HeatLimiter_Apply(float32_t heat_limit, float32_t vbat, float32_t dt_sec) {
	return 0;
}

int8_t PowerLimit_Apply(float32_t power_limit, float32_t vbat, float32_t *motor, uint32_t len) {
	if (motor == NULL)
		return -1;
	
	float32_t total_current = 0.f;
	for(uint32_t i = 0; i < len; i++) {
		total_current += fabs(*motor);
	}
	
	if (power_limit > 0.f) {
		if ((total_current * vbat) > power_limit) {
			float32_t current_scale = total_current / power_limit / vbat;
			for(uint32_t i = 0; i < len; i++) {
				*motor *= current_scale;
			}
		}
	}
	return 0;
}
