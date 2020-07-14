#pragma once


/* Includes ------------------------------------------------------------------*/
#include "user_math.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
int8_t BSP_Buzzer_Start(void);
int8_t BSP_Buzzer_Set(float32_t freq, float32_t duty_cycle);
int8_t BSP_Buzzer_Stop(void);
