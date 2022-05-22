#include "bsp_pwm.h"

#include "main.h"

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim7;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim10;

typedef struct {
  TIM_HandleTypeDef *tim;
  uint16_t channel;
} bsp_pwm_config_t;

static bsp_pwm_config_t BSP_PWM_MAP[BSP_PWM_NUMBER] = {
    [BSP_PWM_IMU_HEAT] = {&htim10, TIM_CHANNEL_1},
    [BSP_PWM_LAUNCHER_SERVO] = {&htim1, TIM_CHANNEL_1},
    [BSP_PWM_BUZZER] = {&htim4, TIM_CHANNEL_3},
    [BSP_PWM_LED_RED] = {&htim5, TIM_CHANNEL_3},
    [BSP_PWM_LED_GRN] = {&htim5, TIM_CHANNEL_2},
    [BSP_PWM_LED_BLU] = {&htim5, TIM_CHANNEL_1},
    [BSP_PWM_LASER] = {&htim3, TIM_CHANNEL_3},
    [BSP_PWM_SERVO_A] = {&htim1, TIM_CHANNEL_1},
    [BSP_PWM_SERVO_B] = {&htim1, TIM_CHANNEL_2},
    [BSP_PWM_SERVO_C] = {&htim1, TIM_CHANNEL_3},
    [BSP_PWM_SERVO_D] = {&htim1, TIM_CHANNEL_4},
    [BSP_PWM_SERVO_E] = {&htim8, TIM_CHANNEL_1},
    [BSP_PWM_SERVO_F] = {&htim8, TIM_CHANNEL_2},
    [BSP_PWM_SERVO_G] = {&htim8, TIM_CHANNEL_3},
};

int8_t bsp_pwm_start(bsp_pwm_channel_t ch) {
  HAL_TIM_PWM_Start(BSP_PWM_MAP[ch].tim, BSP_PWM_MAP[ch].channel);

  return BSP_OK;
}

int8_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle) {
  if (duty_cycle > 1.0f) return BSP_ERR;
  if (duty_cycle < 0.0f) duty_cycle = 0.f;

  /* 通过PWM通道对应定时器重载值和给定占空比，计算PWM周期值 */
  uint16_t pulse = (uint16_t)(duty_cycle * (float)__HAL_TIM_GET_AUTORELOAD(
                                               BSP_PWM_MAP[ch].tim));

  if (pulse > 0) {
    __HAL_TIM_SET_COMPARE(BSP_PWM_MAP[ch].tim, BSP_PWM_MAP[ch].channel, pulse);
  } else {
    bsp_pwm_stop(ch);
  }
  return BSP_OK;
}

int8_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq) {
  uint16_t reload = (uint16_t)(1000000U / freq);

  if (reload > 0) {
    __HAL_TIM_PRESCALER(BSP_PWM_MAP[ch].tim, reload);
  } else {
    return BSP_ERR;
  }

  return BSP_OK;
}

int8_t bsp_pwm_stop(bsp_pwm_channel_t ch) {
  HAL_TIM_PWM_Stop(BSP_PWM_MAP[ch].tim, BSP_PWM_MAP[ch].channel);
  return BSP_OK;
}
