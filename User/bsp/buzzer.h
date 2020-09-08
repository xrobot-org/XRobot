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
int8_t BSP_Buzzer_Start(void);
int8_t BSP_Buzzer_Set(float freq, float duty_cycle);
int8_t BSP_Buzzer_Stop(void);

#ifdef __cplusplus
}
#endif
