#pragma once

#include <stdint.h>

#include "bsp.h"
/* PWM通道 */
typedef enum {
  BSP_PWM_IMU_HEAT,
  BSP_PWM_LAUNCHER_SERVO,
} bsp_pwm_channel_t;

int8_t bsp_pwm_start(bsp_pwm_channel_t ch);
int8_t bsp_pwm_set(bsp_pwm_channel_t ch, float duty_cycle);
int8_t bsp_pwm_stop(bsp_pwm_channel_t ch);
