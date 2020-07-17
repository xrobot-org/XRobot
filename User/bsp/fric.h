#pragma once


/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "component\user_math.h"

/* Exported constants --------------------------------------------------------*/
#define BSP_LAZER_RESOLUTION 10000
#define BSP_LAZER_FREQUENCE 50
#define BSP_LAZER_DEFAULT_DUTY 5000

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
int8_t BSP_Fric_Start(void);
int8_t BSP_Fric_Set(float32_t duty_cycle);
int8_t BSP_Fric_Stop(void);

