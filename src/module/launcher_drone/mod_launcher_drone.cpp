#include "mod_launcher_drone.hpp"

#include "bsp_pwm.h"

#define FRIC_NUM 2
#define TRIG_NUM 1
#define TRIG_MAX_SPEED (8191)
#define FRIC_MAX_SPEED (7500.0f)

using namespace Module;

DroneLauncher::DroneLauncher(Param& param, float control_freq)
    : param_(param), ctrl_lock_(true) {
  for (size_t i = 0; i < TRIG_NUM; i++) {
    this->trig_actr_.at(i) =
        new Component::PosActuator(param.trig_actr.at(i), control_freq);
    this->trig_motor_.at(i) =
        new Device::RMMotor(this->param_.trig_motor.at(i),
                            ("Launcher_Trig" + std::to_string(i)).c_str());
  }

  auto event_callback = [](Event event, DroneLauncher* drone) {
    drone->ctrl_lock_.Wait(UINT32_MAX);
    switch (event) {
      case SET_RELAX:
        drone->SetTrigMode(RELAX);
        drone->SetFricMode(SAFE);
        break;
      case CHANGE_FRIC_MODE_LOADED:
        drone->SetTrigMode(RELAX);
        drone->SetFricMode(LOADED);
        break;
      case CHANGE_FRIC_MODE_SAFE:
        drone->SetTrigMode(RELAX);
        drone->SetFricMode(SAFE);
        break;
      case CHANGE_TRIG_MODE_SINGLE:
        drone->SetTrigMode(SINGLE);
        break;
      case CHANGE_TRIG_MODE_BURST:
        drone->SetTrigMode(BURST);
        break;
      case CHANGE_TRIG_MODE_CONTINUED:
        drone->SetTrigMode(CONTINUED);
        break;
      case SET_START_FIRE:
        drone->SetFricMode(LOADED);
        drone->SetTrigMode(BURST);
        break;
    }
    drone->ctrl_lock_.Post();
  };
  Component::CMD::RegisterEvent<DroneLauncher*, Event>(event_callback, this,
                                                       this->param_.EVENT_MAP);

  auto drone_launcher_thread = [](DroneLauncher* drone) {
    auto ref_sub = Message::Subscriber<Device::Referee::Data>("referee");

    uint32_t last_online_time = bsp_time_get_ms();
    while (1) {
      ref_sub.DumpData(drone->raw_ref_);

      drone->PraseRef();
      drone->ctrl_lock_.Wait(UINT32_MAX);
      drone->Feedback();
      drone->Control();
      drone->ctrl_lock_.Post();
      drone->thread_.SleepUntil(2, last_online_time);
    }
  };
  this->thread_.Create(drone_launcher_thread, this, "drone_launcher_thread",
                       384, System::Thread::MEDIUM);
}

void DroneLauncher::Feedback() {
  float trig_pos_last = this->trig_motor_[0]->GetAngle();
  for (size_t i = 0; i < TRIG_NUM; i++) {
    this->trig_motor_[i]->Update();
  }
  float trig_pos_delta = this->trig_motor_[0]->GetAngle() - trig_pos_last;
  this->trig_pos_ += trig_pos_delta / this->param_.trig_gear_ratio;
}
void DroneLauncher::Control() {
  this->now_ = bsp_time_get();
  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);
  this->last_wakeup_ = this->now_;

  this->FricControl();

  this->TrigControl();
}
void DroneLauncher::FricControl() {
  switch (this->fricmode_) {
    case SAFE:
      bsp_pwm_start(BSP_PWM_SERVO_A);
      bsp_pwm_set_comp(BSP_PWM_SERVO_A, 0.02f);
      bsp_pwm_start(BSP_PWM_SERVO_B);
      bsp_pwm_set_comp(BSP_PWM_SERVO_B, 0.02f);
      break;
    case LOADED:
      bsp_pwm_start(BSP_PWM_SERVO_A);
      bsp_pwm_start(BSP_PWM_SERVO_B);
      bsp_pwm_set_comp(BSP_PWM_SERVO_A, 0.08f);
      bsp_pwm_set_comp(BSP_PWM_SERVO_B, 0.08f);
      break;
  }
}
void DroneLauncher::TrigControl() {
  switch (this->trigmode_) {
    case RELAX:
      this->setpoint_.trig_pos = this->trig_pos_;
      break;
    case SINGLE:
    case BURST:
      if ((bsp_time_get_ms() - this->last_launch_time_) >=
          this->launch_delay_) {
        if ((trig_last_pos_ - trig_pos_) / M_2PI *
                this->param_.bullet_circle_num >
            0.9) {
          if (trig_set_freq_ > 0) {
            if (!stall_) {
              this->trig_last_pos_ = this->setpoint_.trig_pos;
              this->setpoint_.trig_pos -=
                  M_2PI / this->param_.bullet_circle_num;
            }
            trig_set_freq_--;
            this->last_launch_time_ = bsp_time_get_ms();
            this->stall_ = false;
          }
        }
      }
      break;
    case CONTINUED:
      this->continued_rotation_speed_ =
          M_2PI / (8.0f * static_cast<float>(this->param_.min_launcher_delay));
      this->setpoint_.trig_pos -=
          this->continued_rotation_speed_ * dt_ * 1000.0f;
      break;
  }
  switch (this->trigmode_) {
    case RELAX:
      for (size_t i = 0; i < TRIG_NUM; i++) {
        this->trig_motor_[i]->Relax();
      }
      break;
    case SINGLE:
    case BURST:
    case CONTINUED:
      for (size_t i = 0; i < TRIG_NUM; i++) {
        trig_out_ = this->trig_actr_[i]->Calculate(
            this->setpoint_.trig_pos,
            this->trig_motor_[i]->GetSpeed() / TRIG_MAX_SPEED, this->trig_pos_,
            dt_);
        this->trig_motor_[i]->Control(trig_out_);
      }
      break;
  }
}
void DroneLauncher::SetFricMode(FricMode mode) {
  if (this->fricmode_ == mode) {
    return;
  }
  this->fricmode_ = mode;
}
void DroneLauncher::SetTrigMode(TrigMode mode) {
  if (this->trigmode_ == mode) {
    return;
  }
  this->trigmode_ = mode;
  switch (this->trigmode_) {
    case RELAX:
      this->trig_set_freq_ = 0;
      this->launch_delay_ = UINT32_MAX;
      break;
    case SINGLE:
      this->trig_set_freq_ = 1;
      this->launch_delay_ = this->param_.min_launcher_delay;
      break;
    case BURST:
      this->trig_set_freq_ = 8;
      this->launch_delay_ = this->param_.min_launcher_delay;
      break;
    case CONTINUED:
      this->trig_set_freq_ = 0;
      this->launch_delay_ = this->param_.min_launcher_delay;
      break;
  }
}
void DroneLauncher::PraseRef() {
  memcpy(&(this->ref_.robot_status), &(raw_ref_.robot_status),
         sizeof(this->ref_.robot_status));
  this->ref_.status = this->raw_ref_.status;
}
