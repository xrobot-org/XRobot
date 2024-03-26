#include "mod_dart_launcher.hpp"

#include "bsp_time.h"

#define MAX_FRIC_SPEED (7500.0f)

using namespace Module;

DartLauncher::DartLauncher(DartLauncher::Param& param, float control_freq)
    : param_(param),
      rod_actr_(param.rod_actr, control_freq, 500.0f),
      reload_actr_(param.reload_actr, control_freq, 500.0f) {
  this->setpoint_.rod = 0.1;
  this->setpoint_.reload = 0.0f;

  for (int i = 0; i < 4; i++) {
    fric_actr_[i] =
        new Component::SpeedActuator(param.fric_actr[i], control_freq);
    fric_motor_[i] = new Device::RMMotor(
        param.fric_motor[i],
        (std::string("dart_fric_") + std::to_string(i)).c_str());
  }

  auto event_callback = [](Event event, DartLauncher* dart) {
    switch (event) {
      case RELOAD:
        if (bsp_time_get_ms() - dart->last_reload_time_ < 500) {
          return;
        } else {
          dart->last_reload_time_ = bsp_time_get_ms();
          dart->setpoint_.reload += 0.013;
          clampf(&dart->setpoint_.reload, 0, 1.0);
        }
        dart->setpoint_.fric_speed = 0;
        break;
      case RESET:
        dart->setpoint_.reload = 0;
        dart->setpoint_.fric_speed = 0;
        break;
      case ON:
        dart->setpoint_.rod = 1.0f;
        dart->setpoint_.fric_speed = 1.0f;
        break;
      case OFF:
        dart->setpoint_.fric_speed = 0;
        dart->setpoint_.rod = 0.0f;
        break;
    }
  };

  Component::CMD::RegisterEvent<DartLauncher*, Event>(event_callback, this,
                                                      this->param_.EVENT_MAP);

  auto thread_fn = [](DartLauncher* dart) {
    uint32_t last_online_time = bsp_time_get_ms();

    while (true) {
      dart->UpdateFeedback();
      dart->Control();

      dart->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(thread_fn, this, "dart_launcher", 512,
                       System::Thread::MEDIUM);
}

void DartLauncher::UpdateFeedback() {
  this->rod_actr_.UpdateFeedback();
  this->reload_actr_.UpdateFeedback();
  for (auto motor : fric_motor_) {
    motor->Update();
  }
}

void DartLauncher::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  this->rod_actr_.Control(this->setpoint_.rod * this->param_.rod_actr.max_range,
                          dt_);

  this->reload_actr_.Control(
      this->setpoint_.reload * this->param_.reload_actr.max_range, dt_);

  for (int i = 0; i < 4; i++) {
    motor_out_[i] = this->fric_actr_[i]->Calculate(
        this->setpoint_.fric_speed,
        this->fric_motor_[i]->GetSpeed() / MAX_FRIC_SPEED, dt_);
  }

  for (int i = 0; i < 4; i++) {
    this->fric_motor_[i]->Control(motor_out_[i]);
  }
}
