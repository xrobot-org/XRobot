#pragma once

/* Includes ------------------------------------------------------------------*/
#include "board.h"

/* Exported constants --------------------------------------------------------*/
/* Exported defines ----------------------------------------------------------*/
#define PWM_RESOLUTION 10000
#define PWM_FREQUENCE 50
#define PWM_DEFAULT_DUTY 5000

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/



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

int PWM_Start(PWM_Num_t n);
int PWM_Set(PWM_Num_t n, uint16_t pulse);

int Friction_On(uint16_t pulse);
int Friction_Off(void);
