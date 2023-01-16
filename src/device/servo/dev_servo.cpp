#include "dev_servo.hpp"

#include "bsp_pwm.h"

using namespace Device;

Servo::Servo(Channel ch, float max_angle)
    : channel_(ch), max_angle_(max_angle) {}

bool Servo::Start() {
  return bsp_pwm_start(static_cast<bsp_pwm_channel_t>(
             BSP_PWM_SERVO_A + this->channel_)) == BSP_OK;
}

bool Servo::Set(float angle) {
  clampf(&angle, 0.0f, this->max_angle_);
  return bsp_pwm_set_comp(
             static_cast<bsp_pwm_channel_t>(BSP_PWM_SERVO_A + this->channel_),
             (this->max_angle_ - angle) / this->max_angle_ / 10.0f + 0.025f) ==
         BSP_OK;
}

bool Servo::Stop() {
  return bsp_pwm_stop(static_cast<bsp_pwm_channel_t>(BSP_PWM_SERVO_A +
                                                     this->channel_)) == BSP_OK;
}
