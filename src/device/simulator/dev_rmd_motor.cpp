#include "dev_rmd_motor.hpp"

#include <comp_type.hpp>
#include <comp_utils.hpp>

using namespace Device;

RMDMotor::RMDMotor(const Param& param, const char* name)
    : BaseMotor(name), param_(param) {}

void RMDMotor::Control(float output) {
  clampf(&output, -1.0f, 1.0f);
  output *= 4.5;
  if (!param_.reverse) {
    wb_motor_set_torque(this->handle_, output);
  } else {
    wb_motor_set_torque(this->handle_, -output);
  }
}

bool RMDMotor::Update() {
  if (bsp_time_get_ms() == this->last_sensor_time_) {
    return false;
  }

  Component::Type::CycleValue raw_pos =
      static_cast<float>(wb_position_sensor_get_value(this->sensor_));

  this->feedback_.rotational_speed =
      (raw_pos - this->last_pos_) /
      (bsp_time_get_ms() - this->last_sensor_time_) * 1000.0f / M_2PI * 60.0f;

  this->last_pos_ = raw_pos;

  this->feedback_.rotor_abs_angle = raw_pos;

  this->feedback_.temp = 28.0f;

  this->last_online_time_ = bsp_time_get_ms();

  this->last_sensor_time_ = bsp_time_get_ms();

  this->feedback_.torque_current =
      static_cast<float>(wb_motor_get_torque_feedback(this->handle_));

  return true;
}
