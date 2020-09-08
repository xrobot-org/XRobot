/* Includes ------------------------------------------------------------------*/
#include "bsp\fric.h"

#include <tim.h>

#include "bsp\delay.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int8_t BSP_Fric_Start(void) {
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  BSP_Delay(500);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  return BSP_OK;
}
int8_t BSP_Fric_Set(float duty_cycle) {
  uint16_t pulse = (uint16_t)(duty_cycle * (float)UINT16_MAX);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, pulse);
  return BSP_OK;
}

int8_t BSP_Fric_Stop(void) {
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
  return BSP_OK;
}
