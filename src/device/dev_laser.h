#pragma once

#include <stdint.h>

/* duty_cycle大于零时，A板为全开，C板为pwm调光*/
int8_t laser_start(void);
int8_t laser_set(float duty_cycle);
int8_t laser_stop(void);
