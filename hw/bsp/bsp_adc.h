#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"

float bsp_adc_get_cpu_temp(void);
float bsp_adc_get_batt_volt(void);

#ifdef __cplusplus
}
#endif
