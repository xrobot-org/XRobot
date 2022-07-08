#include "dev_buzzer.hpp"

#include "bsp_pwm.h"

using namespace Device;

bool Buzzer::Start() { return bsp_pwm_start(BSP_PWM_BUZZER) == BSP_OK; }

bool Buzzer::Stop() { return bsp_pwm_stop(BSP_PWM_BUZZER) == BSP_OK; }

bool Buzzer::Set(float freq, float duty_cycle) {
  bsp_pwm_set_comp(BSP_PWM_BUZZER, duty_cycle);
  return bsp_pwm_set_freq(BSP_PWM_BUZZER, freq) == BSP_OK;
}
