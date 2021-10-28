#include "dev_buzzer.h"

#include "hal_tim.h"

int8_t Buzzer_Start(void) {
  if (HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3) == HAL_OK) return 0;
  return -1;
}

int8_t Buzzer_Set(float freq, float duty_cycle) {
  uint16_t pulse = (uint16_t)(duty_cycle * (float)UINT16_MAX);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, pulse);

  pulse = (uint16_t)freq;
  __HAL_TIM_PRESCALER(&htim4, pulse);
  return 0;
}

int8_t Buzzer_Stop(void) {
  if (HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3) == HAL_OK) return 0;
  return -1;
}
