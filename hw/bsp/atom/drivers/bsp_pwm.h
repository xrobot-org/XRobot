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

bsp_status_t bsp_pwm_start(bsp_pwm_channel_t ch);
bsp_status_t bsp_pwm_set_comp(bsp_pwm_channel_t ch, float duty_cycle);
bsp_status_t bsp_pwm_set_freq(bsp_pwm_channel_t ch, float freq);
bsp_status_t bsp_pwm_stop(bsp_pwm_channel_t ch);

#ifdef __cplusplus
}
#endif
