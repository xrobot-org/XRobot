#include "mod_dart_launcher.hpp"

#define FRIC_NUM 4
#define FRIC_MAX_SPEED (7500.0f)
#define DART_LEN 0.225

using namespace Module;

DartLauncher::DartLauncher(DartLauncher::Param& param, float control_freq)
    : ctrl_lock_(true), param_(param), rod_(param.rod_, control_freq, 500.0f) {
  for (size_t i = 0; i < FRIC_NUM; i++) {
    fric_actr_[i] =
        new Component::SpeedActuator(param.fric_actr[i], control_freq);
    fric_motor_[i] = new Device::RMMotor(
        param.fric_motor[i],
        (std::string("dart_fric_") + std::to_string(i)).c_str());
  }

  auto event_callback = [](Event event, DartLauncher* dart) {
    dart->ctrl_lock_.Wait(UINT32_MAX);
    switch (event) {
      case SET_MODE_RELAX:
        dart->SetMode(RELAX);
        break;
      case SET_MODE_OFF:
        dart->SetMode(OFF);
        break;
      case SET_MODE_ROD:
        dart->SetMode(ROD_ON);
        break;
      case SET_MODE_FRIC:
        dart->SetMode(FRIC_ON);
        break;
      case SET_MODE_ON:
        dart->SetMode(ON);
        break;
      case SET_MODE_STAY:
        dart->SetMode(STAY);
        break;
      case SET_MODE_ADVANCE:
        dart->SetMode(ADVANCE);
        break;
    }
    dart->ctrl_lock_.Post();
  };
  Component::CMD::RegisterEvent<DartLauncher*, Event>(event_callback, this,
                                                      this->param_.EVENT_MAP);
  auto thread_fn = [](DartLauncher* dart) {
    uint32_t last_online_time = bsp_time_get_ms();
    while (1) {
      dart->ctrl_lock_.Wait(UINT32_MAX);
      dart->Feedback();
      dart->Control();
      dart->ctrl_lock_.Post();
      dart->thread_.SleepUntil(2, last_online_time);
    }
  };
  this->thread_.Create(thread_fn, this, "dart_launcher", 512,
                       System::Thread::MEDIUM);
}
void DartLauncher::Feedback() {
  this->rod_.UpdateFeedback();
  for (size_t i = 0; i < FRIC_NUM; i++) {
    this->fric_motor_[i]->Update();
  }
}
void DartLauncher::Control() {
  this->now_ = bsp_time_get();
  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);
  this->last_wakeup_ = this->now_;

  if (!relax_) {
    this->rod_.Control(this->setpoint_.rod_pos * this->param_.rod_.max_range,
                       dt_);
    for (size_t i = 0; i < FRIC_NUM; i++) {
      if (i % 2) {
        fric_out_[i] = this->fric_actr_[i]->Calculate(
            this->setpoint_.fric_speed2 * FRIC_MAX_SPEED,
            this->fric_motor_[i]->GetSpeed(), dt_);
      } else {
        fric_out_[i] = this->fric_actr_[i]->Calculate(
            this->setpoint_.fric_speed1 * FRIC_MAX_SPEED,
            this->fric_motor_[i]->GetSpeed(), dt_);
      }
    }
    for (size_t i = 0; i < FRIC_NUM; i++) {
      this->fric_motor_[i]->Control(fric_out_[i]);
    }
  } else {
    this->rod_.Relax();
    for (size_t i = 0; i < FRIC_NUM; i++) {
      this->fric_motor_[i]->Relax();
    }
  }
}
void DartLauncher::SetMode(Mode mode) {
  if (mode == this->mode_) {
    return;
  }
  this->mode_ = mode;
  switch (this->mode_) {
    case RELAX:
      this->relax_ = true;
      this->setpoint_.fric_speed1 = 0.0f;
      this->setpoint_.fric_speed2 = 0.0f;
      this->setpoint_.rod_pos = 0.01f;
      break;
    case OFF:
      this->relax_ = false;
      this->setpoint_.fric_speed1 = 0.0f;
      this->setpoint_.fric_speed2 = 0.0f;
      this->setpoint_.rod_pos = 0.01f;
      break;
    case ON:
      this->relax_ = false;
      this->setpoint_.fric_speed1 = 1.0f;
      this->setpoint_.fric_speed1 = 1.0f;
      this->setpoint_.rod_pos = 1.0f;
      break;
    case ROD_ON:
      this->relax_ = false;
      this->setpoint_.fric_speed1 = 0.0f;
      this->setpoint_.fric_speed2 = 0.0f;
      this->setpoint_.rod_pos = 1.0f;
      break;
    case FRIC_ON:
      this->relax_ = false;
      this->setpoint_.fric_speed1 = 1.0f;
      this->setpoint_.fric_speed2 = 1.0f;
      // this->setpoint_.rod_pos = 0.01f;
      break;
    case ADVANCE:
      this->relax_ = false;
      this->setpoint_.fric_speed1 = 1.0f;
      this->setpoint_.fric_speed2 = 1.0f;
      this->setpoint_.rod_pos = static_cast<float>(rod_pos_ * DART_LEN) + 0.1f;
      this->rod_pos_ = ((this->rod_pos_ + 1) % 5);
      break;
    case STAY:
      break;
  }
}
