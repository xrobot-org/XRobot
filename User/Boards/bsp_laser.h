#pragma once


/* Includes ------------------------------------------------------------------*/
#include "user_math.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/* duty_cycle大于零时，A板为全开，C板为pwm调光*/
int8_t BSP_Laser_Start(void);
int8_t BSP_Laser_Set(float32_t duty_cycle);
int8_t BSP_Laser_Stop(void);
