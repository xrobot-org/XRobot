#pragma once

#include <stdint.h>

/* duty_cycle大于零时，A板为全开，C板为pwm调光*/
int8_t Laser_Start(void);
int8_t Laser_Set(float duty_cycle);
int8_t Laser_Stop(void);
