#include <comp_type.hpp>
#pragma once

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_ahrs.hpp"
#include "dev_rm_motor.hpp"
#include "module.hpp"

namespace Module {
class FreeGimbal {
 public:
  typedef enum {
    FORWARD,
    FOLLOW,
  } Mode;

  typedef enum {
    SET_MODE_FORWARD,
    SET_MODE_FOLLOW,
    START_AUTO_AIM,
    STOP_AUTO_AIM
  } GimbalEvent;

  typedef struct {
    Component::PosActuator::Param yaw_actr;

    Device::RMMotor::Param yaw_motor;

    Component::Type::Eulr mech_zero;

    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

  } Param;

  FreeGimbal(Param &param, float control_freq);

  void SetMode(Mode mode);

  void Control();

 private:
  float yaw_out_;

  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;

  float dt_ = 0.0f;

  float yaw_;
  Param param_;

  struct {
    Component::Type::Eulr eulr_; /* 表示云台姿态的欧拉角 */
  } setpoint_;

  FreeGimbal::Mode mode_ = FORWARD; /* 云台模式 */

  Component::PosActuator yaw_actuator_;
  Component::CMD::GimbalCMD cmd_;

  Device::RMMotor yaw_motor_;

  System::Thread thread_;
  System::Semaphore ctrl_lock_;
};
}  // namespace Module
