#pragma once

#include <stdint.h>

#include "bsp.h"

float BSP_GetTemperature(void);
float BSP_GetBatteryVolt(void);
uint8_t BSP_GetHardwareVersion(void);
