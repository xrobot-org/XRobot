#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "bsp.h"
/* PWM通道 */
typedef enum {
  BSP_PWM_IMU_HEAT,
  BSP_PWM_NUMBER,
} bsp_pwm_channel_t;

int8_t bsp_pwm_start(bsp_pwm_channel_t ch);
int8_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle);
int8_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq);
int8_t bsp_pwm_stop(bsp_pwm_channel_t ch);

#ifdef __cplusplus
}
#endif
