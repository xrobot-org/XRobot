#pragma once


/* Includes ------------------------------------------------------------------*/
#include "user_math.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef enum {
	BSP_PWM_IMU_HEAT,
} BSP_PWM_Channel_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t BSP_PWM_Start(BSP_PWM_Channel_t ch);
int8_t BSP_PWM_Set(BSP_PWM_Channel_t ch, float32_t duty_cycle);
int8_t BSP_PWM_Stop(BSP_PWM_Channel_t ch);
