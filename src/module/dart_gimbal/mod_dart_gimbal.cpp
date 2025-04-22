#include "mod_dart_gimbal.hpp"

#include "bsp_time.h"
#include "comp_cmd.hpp"

#define DGIMBAL_MAXYAW_SPEED (M_2PI * 0.01f)
#define DGIMBAL_MAXPIT_SPEED (M_2PI * 0.01f)
using namespace Module;

Dartgimbal::Dartgimbal(Dartgimbal::Param& param, float control_freq)
    : param_(param),
      yaw_(this->param_.yaw_param, control_freq, 500.0f),
      pitch_(this->param_.pitch_param, control_freq, 500.0f),
      ctrl_lock_(true) {
  auto event_callback = [](GimbalEvent event, Dartgimbal* dart_gimbal) {
    dart_gimbal->ctrl_lock_.Wait(UINT32_MAX);
    switch (event) {
      case SET_MODE_RELAX:
        dart_gimbal->SetMode(RELAX);
        break;
      case SET_MODE_STABLE:
        dart_gimbal->SetMode(STABLE);
        break;
      case SET_MODE_CONTROL:
        dart_gimbal->SetMode(CONTROL);
        break;
    }
    dart_gimbal->ctrl_lock_.Post();
  };
  Component::CMD::RegisterEvent<Dartgimbal*, GimbalEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto thread_fn = [](Dartgimbal* dart_gimbal) {
    auto cmd_sub = Message::Subscriber<Component::CMD::GimbalCMD>("cmd_gimbal");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      cmd_sub.DumpData(dart_gimbal->cmd_);

      dart_gimbal->ctrl_lock_.Wait(UINT32_MAX);
      dart_gimbal->UpdateFeedback();
      dart_gimbal->Control();
      dart_gimbal->ctrl_lock_.Post();

      dart_gimbal->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(thread_fn, this, "dartgimbal_thread", 512,
                       System::Thread::MEDIUM);
}

void Dartgimbal::UpdateFeedback() {
  this->pitch_.UpdateFeedback();
  this->yaw_.UpdateFeedback();
}
void Dartgimbal::Control() {
  this->now_ = bsp_time_get();
  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);
  this->last_wakeup_ = this->now_;

  switch (mode_) {
    case RELAX:
      this->pitch_.Relax();
      this->yaw_.Relax();
      break;
    case STABLE:
      this->yaw_.Control(
          this->setpoint_.eulr_.x * this->param_.yaw_param.max_range, dt_);
      this->pitch_.Control(
          this->setpoint_.eulr_.y * this->param_.pitch_param.max_range, dt_);
      break;
    case CONTROL:
      float dart_gimbal_yaw_cmd = 0.0f;
      float dart_gimbal_pit_cmd = 0.0f;

      if (this->cmd_.mode == Component::CMD::GIMBAL_RELATIVE_CTRL) {
        dart_gimbal_yaw_cmd =
            this->cmd_.eulr.yaw * this->dt_ * DGIMBAL_MAXYAW_SPEED;
        dart_gimbal_pit_cmd =
            this->cmd_.eulr.pit * this->dt_ * DGIMBAL_MAXPIT_SPEED;
        this->setpoint_.eulr_.x += dart_gimbal_yaw_cmd;
        this->setpoint_.eulr_.y += dart_gimbal_pit_cmd;
      } else {
        this->setpoint_.eulr_.x = this->cmd_.eulr.yaw;
        this->setpoint_.eulr_.y = this->cmd_.eulr.pit;
      }

      clampf(&this->setpoint_.eulr_.x, 0.0f, 1.0f);
      clampf(&this->setpoint_.eulr_.y, 0.0f, 1.0f);

      this->pitch_.Control(
          this->setpoint_.eulr_.y * this->param_.pitch_param.max_range, dt_);
      this->yaw_.Control(
          this->setpoint_.eulr_.x * this->param_.yaw_param.max_range, dt_);
      break;
  }
}
void Dartgimbal::SetMode(Mode mode) {
  if (mode == this->mode_) {
    return;
  }
  this->mode_ = mode;
  switch (mode_) {
    case RELAX:
      this->setpoint_.eulr_.x = 0.0f;
      this->setpoint_.eulr_.y = 0.0f;
      break;
    case STABLE:
      this->setpoint_.eulr_.x = 0.5f;
      this->setpoint_.eulr_.y = 0.0f;
      break;
    case CONTROL:
      break;
  }
}
