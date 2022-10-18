#include "dev_led.hpp"

#include "bsp_pwm.h"
#include "comp_utils.hpp"

using namespace Device;

static uint32_t led_stats;

LED::LED() {
  auto led_thread = [](LED* led) {
    uint8_t led_fsm = 0;

    while (1) {
      switch (led_fsm) {
        case 0:
          led->Set(GREEN, ON, 1);
          led->Set(RED, OFF, 1);
          led->Set(BLUE, OFF, 1);
          led_fsm++;
          break;
        case 1:
          led->Set(GREEN, OFF, 1);
          led->Set(RED, ON, 1);
          led->Set(BLUE, OFF, 1);
          led_fsm++;
          break;
        case 2:
          led->Set(GREEN, OFF, 1);
          led->Set(RED, OFF, 1);
          led->Set(BLUE, ON, 1);
          led_fsm = 0;
          break;
      }

      led->thread_.Sleep(250);
    }
  };

  this->thread_.Create(led_thread, this, "led_thread", 128,
                       System::Thread::Low);
}

bool LED::Set(LED::Channel ch, LED::Status status, float duty_cycle) {
  clampf(&duty_cycle, 0.0f, 1.0f);

  bsp_pwm_channel_t pwm_ch;

  switch (ch) {
    case RED:
      bsp_pwm_set_comp(BSP_PWM_LED_RED, duty_cycle);
      pwm_ch = BSP_PWM_LED_RED;
      break;

    case GREEN:
      bsp_pwm_set_comp(BSP_PWM_LED_GRN, duty_cycle);
      pwm_ch = BSP_PWM_LED_GRN;
      break;

    case BLUE:
      bsp_pwm_set_comp(BSP_PWM_LED_BLU, duty_cycle);
      pwm_ch = BSP_PWM_LED_BLU;
      break;
    default:
      pwm_ch = BSP_PWM_LED_RED;
  }

  switch (status) {
    case ON:
      bsp_pwm_start(pwm_ch);
      led_stats |= pwm_ch;
      break;

    case OFF:
      bsp_pwm_stop(pwm_ch);
      led_stats &= ~pwm_ch;
      break;

    case TAGGLE:
      if (led_stats & pwm_ch) {
        bsp_pwm_stop(pwm_ch);
        led_stats &= ~pwm_ch;
      } else {
        bsp_pwm_start(pwm_ch);
        led_stats |= pwm_ch;
      }
      break;
  }

  return true;
}
