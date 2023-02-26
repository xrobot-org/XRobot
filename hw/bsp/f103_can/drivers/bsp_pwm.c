#include "bsp_pwm.h"

#include "main.h"

extern TIM_HandleTypeDef htim2;

typedef struct {
  TIM_HandleTypeDef *tim;
  uint16_t channel;
} bsp_pwm_config_t;

static bsp_pwm_config_t bsp_pwm_map[BSP_PWM_NUMBER] = {
    [BSP_PWM_LED_RED] = {&htim2, TIM_CHANNEL_2}};

int8_t bsp_pwm_start(bsp_pwm_channel_t ch) {
  if (ch != BSP_PWM_LED_RED) {
    return BSP_ERR;
  }
  HAL_TIM_PWM_Start(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel);

  return BSP_OK;
}

int8_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle) {
  if (ch != BSP_PWM_LED_RED) {
    return BSP_ERR;
  }

  if (duty_cycle > 1.0f) {
    return BSP_ERR;
  }
  if (duty_cycle < 0.0f) {
    duty_cycle = 0.f;
  }

  /* 通过PWM通道对应定时器重载值和给定占空比，计算PWM周期值 */
  uint16_t pulse = (uint16_t)(duty_cycle * (float)__HAL_TIM_GET_AUTORELOAD(
                                               bsp_pwm_map[ch].tim));

  if (pulse > 0) {
    __HAL_TIM_SET_COMPARE(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel, pulse);
  } else {
    bsp_pwm_stop(ch);
  }
  return BSP_OK;
}

int8_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq) {
  if (ch != BSP_PWM_LED_RED) {
    return BSP_ERR;
  }

  uint16_t reload = (uint16_t)(1000000U / freq);

  if (reload > 0) {
    __HAL_TIM_PRESCALER(bsp_pwm_map[ch].tim, reload);
  } else {
    return BSP_ERR;
  }

  return BSP_OK;
}

int8_t bsp_pwm_stop(bsp_pwm_channel_t ch) {
  if (ch != BSP_PWM_LED_RED) {
    return BSP_ERR;
  }

  HAL_TIM_PWM_Stop(bsp_pwm_map[ch].tim, bsp_pwm_map[ch].channel);
  return BSP_OK;
}
