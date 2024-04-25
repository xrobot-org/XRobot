#include "bsp_pwm.h"

#include "main.h"

extern TIM_HandleTypeDef htim2;

typedef struct {
  TIM_HandleTypeDef *tim;
  uint16_t channel;
} bsp_pwm_config_t;

bsp_status_t bsp_pwm_start(bsp_pwm_channel_t ch) {
  (void)ch;

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

  return BSP_OK;
}

bsp_status_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle) {
  (void)ch;
  (void)duty_cycle;

  if (duty_cycle > 1.0f) {
    duty_cycle = 1.0f;
  }

  if (duty_cycle < 0.0f) {
    duty_cycle = 0.f;
  }
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, duty_cycle * 50);

  return BSP_OK;
}

bsp_status_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq) {
  (void)ch;
  (void)freq;

  return BSP_OK;
}

bsp_status_t bsp_pwm_stop(bsp_pwm_channel_t ch) {
  (void)ch;

  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);

  return BSP_OK;
}
