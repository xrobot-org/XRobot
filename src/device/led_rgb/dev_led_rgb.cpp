#include "dev_led_rgb.hpp"

#include "bsp_pwm.h"

using namespace Device;

RGB::RGB(bool auto_start) {
  bsp_pwm_start(BSP_PWM_LED_RED);
  bsp_pwm_start(BSP_PWM_LED_BLU);
  bsp_pwm_start(BSP_PWM_LED_GRN);

  auto led_thread = [](RGB* led) {
    uint8_t led_fsm = 0;

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      switch (led_fsm) {
        case 0:
          led->Set(GREEN, 1);
          led->Set(RED, 0);
          led->Set(BLUE, 0);
          led_fsm++;
          break;
        case 1:
          led->Set(GREEN, 0);
          led->Set(RED, 1);
          led->Set(BLUE, 0);
          led_fsm++;
          break;
        case 2:
          led->Set(GREEN, 0);
          led->Set(RED, 0);
          led->Set(BLUE, 1);
          led_fsm = 0;
          break;
        default:
          led_fsm = 0;
          break;
      }

      led->thread_.SleepUntil(250, last_online_time);
    }
  };

  if (auto_start) {
    this->thread_.Create(led_thread, this, "led_thread",
                         DEVICE_LED_RGB_TASK_STACK_DEPTH, System::Thread::LOW);
  }
}

bool RGB::Set(RGB::Channel ch, float duty_cycle) {
  clampf(&duty_cycle, 0.0f, 1.0f);

  switch (ch) {
    case RED:
      bsp_pwm_set_comp(BSP_PWM_LED_RED, duty_cycle);
      break;

    case GREEN:
      bsp_pwm_set_comp(BSP_PWM_LED_GRN, duty_cycle);
      break;

    case BLUE:
      bsp_pwm_set_comp(BSP_PWM_LED_BLU, duty_cycle);
      break;
    default:
      break;
  }

  return true;
}
