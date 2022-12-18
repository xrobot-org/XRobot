#include "dev_rm_motor.hpp"

#include "comp_utils.hpp"

#define T_3508 (2.8f)
#define T_2006 (1.0f)
#define T_6020 (1.2f)

using namespace Device;

RMMotor::RMMotor(const Param& param, const char* name)
    : BaseMotor(name), param_(param) {}

void RMMotor::Control(float output) {
  clampf(&output, -1.0f, 1.0f);

  switch (this->param_.model) {
    case MOTOR_M2006:
      output *= T_2006;
      break;
    case MOTOR_M3508:
      output *= T_3508;
      break;
    case MOTOR_GM6020:
      output *= T_6020;
      break;
    default:
      ASSERT(false);
      return;
  }

  wb_motor_set_torque(this->handle_, output);
}

bool RMMotor::Update() {
  float pos = wb_position_sensor_get_value(this->sensor_);

  while (pos < 0.0f) pos += M_2PI;
  while (pos > M_2PI) pos -= M_2PI;

  this->feedback_.rotational_speed =
      circle_error(pos, this->last_pos_, M_2PI) /
      (bsp_time_get() - this->last_sensor_time_) / M_2PI * 60.0f * 19.0f;

  this->last_pos_ = this->feedback_.rotor_abs_angle;

  this->feedback_.rotor_abs_angle = pos;

  this->feedback_.temp = 28.0f;

  this->last_online_time_ = bsp_time_get();

  this->last_sensor_time_ = bsp_time_get();

  this->feedback_.torque_current = wb_motor_get_torque_feedback(this->handle_);

  return true;
}
