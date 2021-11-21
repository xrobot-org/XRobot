#pragma once

#include <stdint.h>

float adc_get_cpu_temp(void);
float adc_get_batt_volt(void);
uint8_t adc_get_hardware_version(void);
