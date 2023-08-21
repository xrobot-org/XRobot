#include "mod_dart_gimbal.hpp"

#include "bsp_time.h"

using namespace Module;

Dartgimbal::Dartgimbal(Dartgimbal::Param& param, float control_freq)
    : param_(param),
      yaw_actr_(param.yaw_actr, control_freq),
      pitch_actr_(this->param_.pitch_actr, control_freq, 500.0f),
      yaw_motor_(param.yaw_motor, "dart_yaw") {
  this->setpoint_.yaw = 0.0;
  this->setpoint_.pitch = 0.0f;

  auto event_callback = [](GimbalEvent event, Dartgimbal* dart) {
    switch (event) {
      case SET_MODE_RELAX:
        dart->setpoint_.pitch = 0;
        dart->setpoint_.yaw = 0;
        break;
      case SET_MODE_ABSOLUTE:
        break;
    }
  };
  Component::CMD::RegisterEvent<Dartgimbal*, GimbalEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto thread_fn = [](Dartgimbal* dart_gimbal) {
    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
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

  yaw_out_ = this->yaw_actr_.Calculate(
      this->setpoint_.yaw, this->yaw_motor_.GetSpeed(),
      this->yaw_motor_.GetAngle() + (M_2PI - this->param_.yaw_zero), dt_);
  this->yaw_motor_.Control(yaw_out_);

  this->pitch_actr_.Control(
      this->setpoint_.pitch * this->param_.pitch_actr.max_range, dt_);
}
