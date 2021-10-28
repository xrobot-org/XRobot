#pragma once

#include <stdint.h>

int8_t Buzzer_Start(void);
int8_t Buzzer_Set(float freq, float duty_cycle);
int8_t Buzzer_Stop(void);
