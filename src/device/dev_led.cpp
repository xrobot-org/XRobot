#include "dev_led.hpp"

#include "bsp_pwm.h"
#include "comp_utils.hpp"

static uint32_t led_stats;

int8_t led_set(led_channel_t ch, led_status_t s, float duty_cycle) {
  ASSERT(duty_cycle <= 1.0f);

  bsp_pwm_channel_t pwm_ch;

  switch (ch) {
    case LED_RED:
      bsp_pwm_set_comp(BSP_PWM_LED_RED, duty_cycle);
      pwm_ch = BSP_PWM_LED_RED;
      break;

    case LED_GRN:
      bsp_pwm_set_comp(BSP_PWM_LED_GRN, duty_cycle);
      pwm_ch = BSP_PWM_LED_GRN;
      break;

    case LED_BLU:
      bsp_pwm_set_comp(BSP_PWM_LED_BLU, duty_cycle);
      pwm_ch = BSP_PWM_LED_BLU;
      break;
    default:
      pwm_ch = BSP_PWM_LED_RED;
  }

  switch (s) {
    case LED_ON:
      bsp_pwm_start(pwm_ch);
      led_stats |= pwm_ch;
      break;

    case LED_OFF:
      bsp_pwm_stop(pwm_ch);
      led_stats &= ~pwm_ch;
      break;

    case LED_TAGGLE:
      if (led_stats & pwm_ch) {
        bsp_pwm_stop(pwm_ch);
        led_stats &= ~pwm_ch;
      } else {
        bsp_pwm_start(pwm_ch);
        led_stats |= pwm_ch;
      }
      break;
  }

  return 0;
}
