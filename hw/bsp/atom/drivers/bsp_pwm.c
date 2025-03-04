#include "bsp_pwm.h"

#include "main.h"

extern TIM_HandleTypeDef htim1, htim2;

typedef struct {
  TIM_HandleTypeDef *tim;
  uint16_t channel;
} bsp_pwm_config_t;

static bsp_pwm_config_t map[BSP_PWM_NUMBER] = {
    {&htim1, TIM_CHANNEL_1},
    {&htim2, TIM_CHANNEL_2},
};

bsp_status_t bsp_pwm_start(bsp_pwm_channel_t ch) {
  HAL_TIM_PWM_Start(map[ch].tim, map[ch].channel);

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
  __HAL_TIM_SET_COMPARE(map[ch].tim, map[ch].channel,
                        (uint16_t)(duty_cycle * 100.0f));

  return BSP_OK;
}

bsp_status_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq) {
  (void)ch;
  (void)freq;

  return BSP_OK;
}

bsp_status_t bsp_pwm_stop(bsp_pwm_channel_t ch) {
  (void)ch;

  HAL_TIM_PWM_Stop(map[ch].tim, map[ch].channel);

  return BSP_OK;
}
