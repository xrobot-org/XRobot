/* Includes ----------------------------------------------------------------- */
#include "bsp\laser.h"

#include <tim.h>

/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private typedef ---------------------------------------------------------- */
/* Private variables -------------------------------------------------------- */
/* Private function  -------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
int8_t BSP_Laser_Start(void) {
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  return BSP_OK;
}

int8_t BSP_Laser_Set(float duty_cycle) {
  if (duty_cycle > 1.f) return BSP_ERR;

  uint16_t pulse = (uint16_t)(duty_cycle * (float)UINT16_MAX);

  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pulse);

  return BSP_OK;
}

int8_t BSP_Laser_Stop(void) {
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
  return BSP_OK;
}
