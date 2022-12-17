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
