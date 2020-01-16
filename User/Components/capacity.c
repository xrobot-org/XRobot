/* 
	电池监视算法。
	
	通过电压值计算剩余电量。
*/

#include "capacity.h"


float Capacity_GetBatteryRemain(float voltage) {
	float percentage;
    float voltage_2 = voltage * voltage;
    float voltage_3 = voltage_2 * voltage;
    
    if(voltage < 19.5f)
        percentage = 0.0f;
	
    else if(voltage < 21.9f)
        percentage = 0.005664f * voltage_3 - 0.3386f * voltage_2 + 6.765f * voltage - 45.17f;
    
    else if(voltage < 25.5f)
        percentage = 0.02269f * voltage_3 - 1.654f * voltage_2 + 40.34f * voltage - 328.4f;
    
    else
        percentage = 1.0f;
    
    if(percentage < 0.0f)
        percentage = 0.0f;
    
    else if(percentage > 1.0f)
        percentage = 1.0f;
    
	
	return percentage;
}


float Capacity_GetCapacitorRemain(float voltage) {
	float percentage;
	
	percentage = (voltage - 19.5f) / (25.5f - 19.5f);
	
    if(percentage < 0.0f)
        percentage = 0.0f;
    
    else if(percentage > 1.0f)
        percentage = 1.0f;
    
	
	return percentage;
}
