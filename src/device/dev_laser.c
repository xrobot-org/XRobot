#include "dev_laser.h"

#include "bsp_pwm.h"

int8_t laser_start(void) {
  bsp_pwm_start(BSP_PWM_LASER);
  return 0;
}

int8_t laser_set(float duty_cycle) {
  if (duty_cycle > 1.0f) return -1;

  bsp_pwm_set_comp(BSP_PWM_LASER, duty_cycle);

  return 0;
}

int8_t laser_stop(void) {
  bsp_pwm_stop(BSP_PWM_LASER);
  return 0;
}
