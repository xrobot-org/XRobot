#include "mod_launcher_drone.hpp"

#include <comp_actuator.hpp>
#include <cstdint>

#include "bsp_pwm.h"
#include "bsp_time.h"
#define LAUNCHER_TRIG_SPEED_MAX (8191)

using namespace Module;
UVALauncher::UVALauncher(Param& param, float control_freq)
    : param_(param), ctrl_lock_(true) {
  for (size_t i = 0; i < 1; i++) {
    this->trig_actuator_.at(i) =
        new Component::PosActuator(param.trig_actr.at(i), control_freq);

    this->trig_motor_.at(i) =
        new Device::RMMotor(this->param_.trig_motor.at(i),
                            ("Launcher_Trig" + std::to_string(i)).c_str());
  }

  auto event_callback = [](LauncherEvent event, UVALauncher* launcher) {
    switch (event) {
      case CHANGE_FIRE_MODE_SAFE:

        launcher->SetTrigMode(SAFE);
        launcher->fire_ctrl_.fire = false;
        launcher->fire_ctrl_.firc_on = false;

        break;
      case CHANGE_TRIG_MODE_SINGLE:

        launcher->fire_ctrl_.fire = true;
        launcher->fire_ctrl_.firc_on = true;

        launcher->SetTrigMode(SINGLE);
        break;

      case CHANGE_TRIG_MODE_BURST:

        launcher->fire_ctrl_.fire = false;
        launcher->fire_ctrl_.firc_on = false;

        launcher->SetTrigMode(BURST);
        break;

      default:
        break;
    }
  };

  Component::CMD::RegisterEvent<UVALauncher*, LauncherEvent>(
      event_callback, this, this->param_.EVENT_MAP);

  auto launcher_thread = [](UVALauncher* launcher) {
    auto ref_sub = Message::Subscriber<Device::Referee::Data>("referee");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      ref_sub.DumpData(launcher->raw_ref_);

      launcher->PraseRef();
      launcher->ctrl_lock_.Wait(UINT32_MAX);

      launcher->FricControl();
      launcher->Control();

      launcher->ctrl_lock_.Post();

      /* 运行结束，等待下一次唤醒 */
      launcher->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(launcher_thread, this, "launcher_thread", 384,
                       System::Thread::MEDIUM);
}

void UVALauncher::SetTrigMode(TrigMode mode) {
  if (mode == this->fire_ctrl_.trig_mode_) {
    return;
  }

  this->fire_ctrl_.trig_mode_ = mode;
}
void UVALauncher::Control() {
  const float LAST_TRIG_MOTOR_ANGLE = this->trig_motor_[0]->GetAngle();

  for (size_t i = 0; i < LAUNCHER_ACTR_TRIG_NUM; i++) {
    this->trig_motor_[i]->Update();
  }

  const float DELTA_MOTOR_ANGLE =
      this->trig_motor_[0]->GetAngle() - LAST_TRIG_MOTOR_ANGLE;

  this->trig_angle_ += DELTA_MOTOR_ANGLE / this->param_.trig_gear_ratio;

  this->now_ = bsp_time_get();
  this->dt_ = this->now_ - this->last_wakeup_;
  this->last_wakeup_ = this->now_;

  /* 根据开火模式计算发射行为 */
  uint32_t max_burst = 0;
  switch (this->fire_ctrl_.trig_mode_) {
    case SINGLE: /* 点射开火模式 */
      max_burst = 1;
      break;
    case BURST: /* 爆发开火模式 */
      max_burst = 8;
      break;
    default:
      max_burst = 0;
      break;
  }

  switch (this->fire_ctrl_.trig_mode_) {
    case SINGLE: /* 点射开火模式 */
    case BURST:  /* 爆发开火模式 */
      /* 计算是否是第一次按下开火键 */
      this->fire_ctrl_.first_pressed_fire =
          this->fire_ctrl_.fire && !this->fire_ctrl_.last_fire;
      this->fire_ctrl_.last_fire = this->fire_ctrl_.fire;

      /* 设置要发射多少弹丸 */
      if (this->fire_ctrl_.first_pressed_fire && !this->fire_ctrl_.to_launch) {
        this->fire_ctrl_.to_launch = max_burst;
        this->fire_ctrl_.fire = false;
      }

      this->fire_ctrl_.launch_delay = this->param_.min_launch_delay;

      break;
    case SAFE:
      this->fire_ctrl_.launch_delay = UINT32_MAX;

      break;
    default:
      break;
  }

  /* 计算拨弹电机位置的目标值 */
  if ((bsp_time_get_ms() - this->fire_ctrl_.last_launch) >=
          this->fire_ctrl_.launch_delay &&
      this->fire_ctrl_.to_launch) {
    if ((fire_ctrl_.last_trig_angle - trig_angle_) / M_2PI *
            this->param_.num_trig_tooth >
        0.9) {
      fire_ctrl_.last_trig_angle = this->setpoint_.trig_angle_;
      /* 将拨弹电机角度进行循环加法，每次加(减)射出一颗弹丸的弧度变化 */
      this->setpoint_.trig_angle_ -= M_2PI / this->param_.num_trig_tooth;
      this->fire_ctrl_.to_launch--;
      this->fire_ctrl_.last_launch = bsp_time_get_ms();
    }
  }

  for (int i = 0; i < 1; i++) {
    /* 控制拨弹电机 */
    float trig_out = this->trig_actuator_[i]->Calculate(
        this->setpoint_.trig_angle_,
        this->trig_motor_[i]->GetSpeed() / LAUNCHER_TRIG_SPEED_MAX,
        this->trig_angle_, this->dt_);

    this->trig_motor_[i]->Control(trig_out);
  }
}
void UVALauncher::PraseRef() {
  memcpy(&(this->ref_.robot_status), &(this->raw_ref_.robot_status),
         sizeof(this->ref_.robot_status));

  this->ref_.status = this->raw_ref_.status;
}
void UVALauncher::FricControl() {
  if (/*this->raw_ref_.robot_status.power_launcher_output == 1 &&*/
      this->fire_ctrl_.firc_on == true) {
    bsp_pwm_start(BSP_PWM_SERVO_A);
    bsp_pwm_start(BSP_PWM_SERVO_B);

    bsp_pwm_set_comp(BSP_PWM_SERVO_A, 0.09f);
    bsp_pwm_set_comp(BSP_PWM_SERVO_B, 0.09f);
  } else {
    bsp_pwm_start(BSP_PWM_SERVO_A);
    bsp_pwm_set_comp(BSP_PWM_SERVO_A, 0.02f);
    bsp_pwm_start(BSP_PWM_SERVO_B);
    bsp_pwm_set_comp(BSP_PWM_SERVO_B, 0.02f);
  }
}
