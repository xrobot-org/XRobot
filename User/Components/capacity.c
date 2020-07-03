/* 
	剩余电量算法。
	
	通过电压值计算剩余电量。
*/

#include "capacity.h"


float32_t Capacity_GetBatteryRemain(float32_t volt) {
	float32_t percentage;
	float32_t volt_2 = volt * volt;
	float32_t volt_3 = volt_2 * volt;
	
	if(volt < 19.5f)
		percentage = 0.0f;
	
	else if(volt < 21.9f)
		percentage = 0.005664f * volt_3 - 0.3386f * volt_2 + 6.765f * volt - 45.17f;
	
	else if(volt < 25.5f)
		percentage = 0.02269f * volt_3 - 1.654f * volt_2 + 40.34f * volt - 328.4f;
	
	else
		percentage = 1.0f;
	
	if(percentage < 0.0f)
		percentage = 0.0f;
	
	else if(percentage > 1.0f)
		percentage = 1.0f;
	
	
	return percentage;
}


float32_t Capacity_GetCapacitorRemain(float32_t volt) {
	float32_t percentage;
	
	percentage = (volt - 19.5f) / (25.5f - 19.5f);
	
	if(percentage < 0.0f)
		percentage = 0.0f;
	
	else if(percentage > 1.0f)
		percentage = 1.0f;
	
	
	return percentage;
}
