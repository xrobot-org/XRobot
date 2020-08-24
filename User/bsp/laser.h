#pragma once

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

#include "bsp/bsp.h"

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/* duty_cycle大于零时，A板为全开，C板为pwm调光*/
int8_t BSP_Laser_Start(void);
int8_t BSP_Laser_Set(float duty_cycle);
int8_t BSP_Laser_Stop(void);

#ifdef __cplusplus
}
#endif
