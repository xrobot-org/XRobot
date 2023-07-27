#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"
/* PWM通道 */
typedef enum {
  BSP_PWM_IMU_HEAT,
  BSP_PWM_LAUNCHER_SERVO,
  BSP_PWM_BUZZER,
  BSP_PWM_LED_RED,
  BSP_PWM_LED_GRN,
  BSP_PWM_LED_BLU,
  BSP_PWM_LASER,
  BSP_PWM_SERVO_A,
  BSP_PWM_SERVO_B,
  BSP_PWM_SERVO_C,
  BSP_PWM_SERVO_D,
  BSP_PWM_SERVO_E,
  BSP_PWM_SERVO_F,
  BSP_PWM_SERVO_G,
  BSP_PWM_NUMBER,
} bsp_pwm_channel_t;

bsp_status_t bsp_pwm_start(bsp_pwm_channel_t ch);
bsp_status_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle);
bsp_status_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq);
bsp_status_t bsp_pwm_stop(bsp_pwm_channel_t ch);

#ifdef __cplusplus
}
#endif
