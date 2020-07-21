#pragma once


/* Includes ------------------------------------------------------------------*/
#include "component\user_math.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
int8_t BSP_Buzzer_Start(void);
int8_t BSP_Buzzer_Set(float freq, float duty_cycle);
int8_t BSP_Buzzer_Stop(void);
