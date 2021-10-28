#pragma once

#include <stdint.h>

#include "bsp.h"

int8_t BSP_Buzzer_Start(void);
int8_t BSP_Buzzer_Set(float freq, float duty_cycle);
int8_t BSP_Buzzer_Stop(void);
