#pragma once

#include <stdint.h>

int8_t buzzer_start(void);
int8_t buzzer_set(float freq, float duty_cycle);
int8_t buzzer_stop(void);
