#include "dev_buzzer.hpp"

#include "bsp_pwm.h"

int8_t buzzer_start(void) {
  if (bsp_pwm_start(BSP_PWM_BUZZER) == BSP_OK) return 0;
  return -1;
}

int8_t buzzer_set(float freq, float duty_cycle) {
  bsp_pwm_set_comp(BSP_PWM_BUZZER, duty_cycle);
  bsp_pwm_set_freq(BSP_PWM_BUZZER, freq);

  return 0;
}

int8_t buzzer_stop(void) {
  if (bsp_pwm_start(BSP_PWM_BUZZER) == BSP_OK) return 0;
  return -1;
}
