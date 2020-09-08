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
typedef enum {
  BSP_PWM_IMU_HEAT,
} BSP_PWM_Channel_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t BSP_PWM_Start(BSP_PWM_Channel_t ch);
int8_t BSP_PWM_Set(BSP_PWM_Channel_t ch, float duty_cycle);
int8_t BSP_PWM_Stop(BSP_PWM_Channel_t ch);

#ifdef __cplusplus
}
#endif
