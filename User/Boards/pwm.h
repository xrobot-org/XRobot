#pragma once

#include "board.h"

typedef enum {
	PWM_A,
	PWM_B,
	PWM_C,
	PWM_D,
	PWM_E,
	PWM_F,
	PWM_G,
	PWM_H,
	PWM_S,
	PWM_T,
	PWM_U,
	PWM_V,
	PWM_W,
	PWM_X,
	PWM_Y,
	PWM_Z,
	PWM_IMU_HEAT,
} PWM_Num_t;

Board_Status_t PWM_Start(PWM_Num_t n);
Board_Status_t PWM_Set(PWM_Num_t n, uint16_t pulse);

Board_Status_t Friction_On(uint16_t pulse);
Board_Status_t Friction_Off(void);
