#include "dev_fric.h"

#include "bsp_delay.h"
#include "hal_tim.h"

int8_t fric_start(void) {
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  BSP_Delay(500);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  return 0;
}
int8_t fric_set(float duty_cycle) {
  uint16_t pulse = (uint16_t)(duty_cycle * (float)UINT16_MAX);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, pulse);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, pulse);
  return 0;
}

int8_t fric_stop(void) {
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
  return 0;
}
