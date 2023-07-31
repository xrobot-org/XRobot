#include "bsp_pwm.h"

#include "main.h"

extern TIM_HandleTypeDef htim2;

typedef struct {
  TIM_HandleTypeDef *tim;
  uint16_t channel;
} bsp_pwm_config_t;

bsp_status_t bsp_pwm_start(bsp_pwm_channel_t ch) {
  (void)ch;

  return BSP_OK;
}

bsp_status_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle) {
  (void)ch;
  (void)duty_cycle;

  return BSP_OK;
}

bsp_status_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq) {
  (void)ch;
  (void)freq;

  return BSP_OK;
}

bsp_status_t bsp_pwm_stop(bsp_pwm_channel_t ch) {
  (void)ch;

  return BSP_OK;
}
