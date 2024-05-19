#include "mod_dart_launcher.hpp"

#include "bsp_time.h"

#define MAX_FRIC_SPEED (7500.0f)
#define DART_LEN 0.225

using namespace Module;

DartLauncher::DartLauncher(DartLauncher::Param& param, float control_freq)
    : ctrl_lock_(true),
      param_(param),
      rod_actr_(param.rod_actr, control_freq, 500.0f) {
  this->setpoint_.rod_position = 0.1f;

  for (int i = 0; i < 4; i++) {
    fric_actr_[i] =
        new Component::SpeedActuator(param.fric_actr[i], control_freq);
    fric_motor_[i] = new Device::RMMotor(
        param.fric_motor[i],
        (std::string("dart_fric_") + std::to_string(i)).c_str());
  }

  auto event_callback = [](DartEvent event, DartLauncher* dart) {
    dart->ctrl_lock_.Wait(UINT32_MAX);
    switch (event) {
      case RELAX:
        dart->SetFricMode(FRIC_OFF);
        break;
      case SET_FRIC_OUTOST:
        dart->SetFricMode(FRIC_OUTOST);
        break;
      case SET_FRIC_BASE:
        dart->SetFricMode(FRIC_BASE);
        break;
      case SET_FRIC_OFF:
        dart->SetFricMode(FRIC_OFF);
        break;
      case SET_STAY:
        dart->SetRodMode(STAY);
        break;
      case RESET_POSITION:
        dart->SetRodMode(BACK);
        break;
      case FIRE:
        dart->SetRodMode(ADVANCE);
      default:
        break;
    }
    dart->ctrl_lock_.Post();
  };

  Component::CMD::RegisterEvent<DartLauncher*, DartEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto thread_fn = [](DartLauncher* dart) {
    uint32_t last_online_time = bsp_time_get_ms();

    while (true) {
      dart->ctrl_lock_.Wait(UINT32_MAX);

      dart->UpdateFeedback();
      dart->Control();

      dart->ctrl_lock_.Post();

      dart->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(thread_fn, this, "dart_launcher", 512,
                       System::Thread::MEDIUM);
}

void DartLauncher::UpdateFeedback() {
  this->rod_actr_.UpdateFeedback();
  for (size_t i = 0; i < 4; i++) {
    this->fric_motor_[i]->Update();
  }
}

void DartLauncher::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  switch (this->fric_mode_) {
    case FRIC_OUTOST:
      this->setpoint_.fric_speed = 0.933f;
      break;
    case FRIC_BASE:
      this->setpoint_.fric_speed = 1.0f;
      break;
    case FRIC_OFF:
      this->setpoint_.fric_speed = 0.0f;
      break;
  }
  switch (this->rod_mode_) {
    case ADVANCE:
      if (this->fric_mode_ == FRIC_OUTOST ||
          this->fric_mode_ ==
              FRIC_BASE) { /*只有在摩擦轮开启状态下飞镖被向前推进*/
        this->setpoint_.rod_position =
            (static_cast<float>(rod_position_)) * DART_LEN + 0.1;
      }
      break;
    case STAY:
      break;
    case BACK:
      this->setpoint_.rod_position = 0.1f;
      if (rod_position_ == 4) {
        rod_position_ = 0;
      }
  }
  /* 上电自动校准 和控制电机输出 */
  this->rod_actr_.Control(
      this->setpoint_.rod_position * this->param_.rod_actr.max_range, dt_);

  /* fric */
  for (int i = 0; i < 4; i++) {
    motor_out_[i] = this->fric_actr_[i]->Calculate(
        this->setpoint_.fric_speed * MAX_FRIC_SPEED,
        this->fric_motor_[i]->GetSpeed(), dt_);
  }
  for (int i = 0; i < 4; i++) {
    this->fric_motor_[i]->Control(motor_out_[i]);
  }
}
/*判断是否切换模式*/
void DartLauncher::SetRodMode(RodMode mode) {
  if (mode == this->rod_mode_) { /* 未更改，return */
    return;
  } else {
    this->rod_mode_ = mode;
    /*如果切换到前进模式，位置向前推进*/
    if (mode == ADVANCE &&
        (this->fric_mode_ == FRIC_OUTOST || this->fric_mode_ == FRIC_BASE)) {
      rod_position_ = ((rod_position_ + 1) % 5);
    }
  }
}

void DartLauncher::SetFricMode(FricMode mode) {
  if (mode == this->fric_mode_) { /* 未更改，return */
    return;
  }
  this->fric_mode_ = mode;
}
