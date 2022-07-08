#include "dev_laser.hpp"

#include "bsp_pwm.h"

using namespace Device;

void Laser::Start() { bsp_pwm_start(BSP_PWM_LASER); }

bool Laser::Set(float duty_cycle) {
  if (duty_cycle > 1.0f) return false;

  bsp_pwm_set_comp(BSP_PWM_LASER, duty_cycle);

  return true;
}

void Laser::Stop() { bsp_pwm_stop(BSP_PWM_LASER); }
