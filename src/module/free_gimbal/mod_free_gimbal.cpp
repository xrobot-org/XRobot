#include "mod_free_gimbal.hpp"

#include <comp_cmd.hpp>
#include <comp_type.hpp>

#include "bsp_time.h"

using namespace Module;

#define GIMBAL_MAX_SPEED (M_2PI * 1.5f)

FreeGimbal::FreeGimbal(Param& param, float control_freq)
    : param_(param),
      yaw_actuator_(this->param_.yaw_actr, control_freq),
      yaw_motor_(this->param_.yaw_motor, "Gimbal_Yaw"),
      ctrl_lock_(true) {
  auto event_callback = [](GimbalEvent event, FreeGimbal* gimbal) {
    gimbal->ctrl_lock_.Wait(UINT32_MAX);

    switch (event) {
      case SET_MODE_FORWARD:
      case SET_MODE_FOLLOW:
        gimbal->SetMode(static_cast<Mode>(event));
        break;
      case START_AUTO_AIM:
        Component::CMD::SetCtrlSource(Component::CMD::CTRL_SOURCE_AI);
        break;
      case STOP_AUTO_AIM:
        Component::CMD::SetCtrlSource(Component::CMD::CTRL_SOURCE_RC);
        break;
    }
    gimbal->ctrl_lock_.Post();
  };
  Component::CMD::RegisterEvent<FreeGimbal*, GimbalEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto gimbal_thread = [](FreeGimbal* gimbal) {
    auto cmd_sub = Message::Subscriber<Component::CMD::GimbalCMD>("cmd_gimbal");
    gimbal->cmd_.mode = Component::CMD::GIMBAL_RELATIVE_CTRL;
    while (1) {
      cmd_sub.DumpData(gimbal->cmd_);

      gimbal->ctrl_lock_.Wait(UINT32_MAX);
      gimbal->Control();
      gimbal->ctrl_lock_.Post();

      /* 运行结束，等待下一次唤醒 */
      gimbal->thread_.SleepUntil(2);
    }
  };

  this->thread_.Create(gimbal_thread, this, "gimbal_thread", 512,
                       System::Thread::MEDIUM);
}

void FreeGimbal::SetMode(Mode mode) {
  if (mode == this->mode_) {
    return;
  }
  this->mode_ = mode;
}

void FreeGimbal::Control() {
  this->yaw_motor_.Update();
  this->yaw_ = this->yaw_motor_.GetAngle();

  this->now_ = bsp_time_get();
  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;
  float gimbal_yaw_cmd = 0.0f;

  if (this->cmd_.mode == Component::CMD::GIMBAL_RELATIVE_CTRL) {
    gimbal_yaw_cmd = this->cmd_.eulr.yaw * this->dt_ * GIMBAL_MAX_SPEED;

  } else {
    gimbal_yaw_cmd = Component::Type::CycleValue(this->cmd_.eulr.yaw) -
                     this->setpoint_.eulr_.yaw;
  }
  this->setpoint_.eulr_.yaw += gimbal_yaw_cmd;

  switch (this->mode_) {
    case FORWARD:

      yaw_out_ = this->yaw_actuator_.Calculate(
          3.13392282, 0, this->yaw_motor_.GetAngle(), this->dt_);

      break;
    case FOLLOW:

      yaw_out_ = this->yaw_actuator_.Calculate(
          this->setpoint_.eulr_.yaw, 0, this->yaw_motor_.GetAngle(), this->dt_);

      break;
  }
  this->yaw_motor_.Control(yaw_out_);
}
