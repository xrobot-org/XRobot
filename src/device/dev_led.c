#include "dev_led.h"

#include "comp_utils.h"
#include "hal_tim.h"

static uint32_t led_stats;

int8_t LED_Set(LED_Channel_t ch, LED_Status_t s, float duty_cycle) {
  ASSERT(duty_cycle <= 1.0f);

  uint32_t tim_ch;
  uint16_t pulse = (uint16_t)(duty_cycle * (float)UINT16_MAX);

  switch (ch) {
    case LED_RED:
      __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_3, pulse);
      tim_ch = TIM_CHANNEL_3;
      break;

    case LED_GRN:
      __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_2, pulse);
      tim_ch = TIM_CHANNEL_2;
      break;

    case LED_BLU:
      __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_1, pulse);
      tim_ch = TIM_CHANNEL_1;
      break;
    default:
      tim_ch = TIM_CHANNEL_1;
  }

  switch (s) {
    case LED_ON:
      HAL_TIM_PWM_Start(&htim5, tim_ch);
      led_stats |= tim_ch;
      break;

    case LED_OFF:
      HAL_TIM_PWM_Stop(&htim5, tim_ch);
      led_stats &= ~tim_ch;
      break;

    case LED_TAGGLE:
      if (led_stats & tim_ch) {
        HAL_TIM_PWM_Stop(&htim5, tim_ch);
        led_stats &= ~tim_ch;
      } else {
        HAL_TIM_PWM_Start(&htim5, tim_ch);
        led_stats |= tim_ch;
      }
      break;
  }

  return 0;
}
