#include "mod_dart_gimbal.hpp"

#include "bsp_time.h"
#include "comp_cmd.hpp"

#define DGIMBAL_MAXYAW_SPEED (M_2PI * 1.5f)
#define DGIMBAL_MAXPIT_SPEED (M_2PI * 0.1f)
using namespace Module;

Dartgimbal::Dartgimbal(Dartgimbal::Param& param, float control_freq)
    : param_(param),
      yaw_actr_(param.yaw_actr, control_freq),
      pitch_actr_(this->param_.pitch_actr, control_freq, 500.0f),
      yaw_motor_(param.yaw_motor, "dart_yaw") {
  auto event_callback = [](GimbalEvent event, Dartgimbal* dart) {
    switch (event) {
      case SET_MODE_RELAX:
        dart->setpoint_.eulr_.yaw = 0.5f;
        dart->setpoint_.eulr_.pit = 0.0f;
        break;
      case SET_MODE_ABSOLUTE:
        break;
    }
  };
  Component::CMD::RegisterEvent<Dartgimbal*, GimbalEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto thread_fn = [](Dartgimbal* dart_gimbal) {
    auto cmd_sub = Message::Subscriber<Component::CMD::GimbalCMD>("cmd_gimbal");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      cmd_sub.DumpData(dart_gimbal->cmd_);

      dart_gimbal->UpdateFeedback();
      dart_gimbal->Control();

      dart_gimbal->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(thread_fn, this, "dartgimbal_thread", 512,
                       System::Thread::MEDIUM);
}

void Dartgimbal::UpdateFeedback() {
  this->pitch_actr_.UpdateFeedback();
  this->yaw_motor_.Update();
}
void Dartgimbal::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  float dart_gimbal_yaw_cmd = 0.0f;
  float dart_gimbal_pit_cmd = 0.0f;

  if (this->cmd_.mode == Component::CMD::GIMBAL_RELATIVE_CTRL) {
    dart_gimbal_yaw_cmd =
        this->cmd_.eulr.yaw * this->dt_ * DGIMBAL_MAXYAW_SPEED;
    dart_gimbal_pit_cmd =
        this->cmd_.eulr.pit * this->dt_ * DGIMBAL_MAXPIT_SPEED;
    this->setpoint_.eulr_.yaw += dart_gimbal_yaw_cmd;
    this->setpoint_.eulr_.pit += dart_gimbal_pit_cmd;
  } else {
    this->setpoint_.eulr_.yaw = this->cmd_.eulr.yaw;
    this->setpoint_.eulr_.pit = this->cmd_.eulr.pit;
  }

  yaw_eulr_ = this->setpoint_.eulr_.yaw;
  pit_eulr_ = this->setpoint_.eulr_.pit;

  clampf(&yaw_eulr_, 0.1, 6.18);

  clampf(&pit_eulr_, 0, 1);

  float yaw_out = this->yaw_actr_.Calculate(
      yaw_eulr_, this->yaw_motor_.GetSpeed(),
      (this->yaw_motor_.GetAngle() + (M_2PI - this->param_.yaw_zero)), dt_);
  this->yaw_motor_.Control(yaw_out);
  this->pitch_actr_.Control(pit_eulr_ * this->param_.pitch_actr.max_range, dt_);
}
