/**
 * @file chassis.c
 * @author Qu Shen (503578404@qq.com)
 * @brief 底盘模组
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#include "mod_Radar.hpp"

#include <math.h>

#include <random>

#include "bsp_time.h"

#define MOTOR_MAX_SPEED_COFFICIENT 1.2f /* 电机的最大转速 */

using namespace Module;

template <typename Motor, typename MotorParam>
Radar<Motor, MotorParam>::Radar(Param& param, float control_freq)
    : param_(param),
      mode_(Radar_car::RELAX),
      mixer_(param.type),
      ctrl_lock_(true) {
  memset(&(this->cmd_), 0, sizeof(this->cmd_));

  for (uint8_t i = 0; i < this->mixer_.len_; i++) {
    this->actuator_.at(i) =
        new Component::SpeedActuator(param.actuator_param.at(i), control_freq);

    this->motor_.at(i) =
        new Motor(param.motor_param.at(i),
                  (std::string("Chassis_") + std::to_string(i)).c_str());
  }

  this->setpoint_.motor_rotational_speed =
      reinterpret_cast<float*>(System::Memory::Malloc(
          this->mixer_.len_ * sizeof(*this->setpoint_.motor_rotational_speed)));
  XB_ASSERT(this->setpoint_.motor_rotational_speed);

  auto event_callback = [](ChassisEvent event, Radar_car* chassis) {
    chassis->ctrl_lock_.Wait(UINT32_MAX);

    switch (event) {
      case SET_MODE_RELAX:
        chassis->SetMode(RELAX);
        break;
      case SET_MODE_INDENPENDENT:
        chassis->SetMode(INDENPENDENT);
        break;
      default:
        break;
    }

    chassis->ctrl_lock_.Post();
  };

Component::CMD::RegisterEvent<Radar_car*, ChassisEvent>(event_callback, this,
                                                        this->param_.EVENT_MAP);

  auto chassis_thread = [](Radar_car* chassis) {
    auto cmd_sub =
        Message::Subscriber<Component::CMD::ChassisCMD>("cmd_chassis");

    uint32_t last_online_time = bsp_time_get_ms();

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      cmd_sub.DumpData(chassis->cmd_);

      /* 更新反馈值 */

      chassis->ctrl_lock_.Wait(UINT32_MAX);
      chassis->UpdateFeedback();
      chassis->Control();
      chassis->ctrl_lock_.Post();

      /* 运行结束，等待下一次唤醒 */
      chassis->thread_.SleepUntil(2, last_online_time);
    }
  };

  this->thread_.Create(chassis_thread, this, "chassis_thread",
                       1024, System::Thread::MEDIUM);

}

template <typename Motor, typename MotorParam>
void Radar<Motor, MotorParam>::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->motor_[i]->Update();
    this->motor_feedback_[i] = this->motor_[i]->GetSpeed();
  }
}

// template <typename Motor, typename MotorParam>
// uint16_t Radar<Motor, MotorParam>::MAXSPEEDGET(float power_limit) {
//   if (param_.get_speed) {
//     return param_.get_speed(power_limit);
//   } else {
//     return 5000;
//   }
// }

template <typename Motor, typename MotorParam>
void Radar<Motor, MotorParam>::Control() {
  this->now_ = bsp_time_get();

  this->dt_ = TIME_DIFF(this->last_wakeup_, this->now_);

  this->last_wakeup_ = this->now_;

  max_motor_rotational_speed_ = 5000.0f;
  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 计算vx、vy */
  switch (this->mode_) {
    case Radar_car::BREAK: /* 刹车模式电机停止 */
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;

    case Radar_car::INDENPENDENT: /* 独立模式控制向量与运动向量相等
                                 */
      this->move_vec_.vx = this->cmd_.x;
      this->move_vec_.vy = this->cmd_.y;
      this->move_vec_.wz = -this->cmd_.z;
      break;

    case Radar_car::RELAX:
    default:
      break;
  }

  /* move_vec -> motor_rpm_set. 通过运动向量计算轮子转速目标值 */
  this->mixer_.Apply(this->move_vec_, this->setpoint_.motor_rotational_speed);

  /* 根据轮子转速目标值，利用PID计算电机输出值 */

  /* 根据底盘模式计算输出值 */
  switch (this->mode_) {
    case Radar_car::BREAK:
    case Radar_car::INDENPENDENT: /* 独立模式,受PID控制 */ {

      for (unsigned i = 0; i < this->mixer_.len_; i++) {
        out_.motor_out[i] = this->actuator_[i]->Calculate(
            this->setpoint_.motor_rotational_speed[i] *
                max_motor_rotational_speed_,
            this->motor_[i]->GetSpeed(), this->dt_);

      }
      for(unsigned i = 0;i < this->mixer_.len_;i++) {
        this->motor_[i]->Control(out_.motor_out[i]);
      }
      break;
    }

    case Radar_car::RELAX: /* 放松模式,不输出 */
      for (size_t i = 0; i < this->mixer_.len_; i++) {
        this->motor_[i]->Relax();
      }
      break;
    default:
      XB_ASSERT(false);
      return;
  }
}

template <typename Motor, typename MotorParam>
void Radar<Motor, MotorParam>::SetMode(Radar::Mode mode) {
  if (mode == this->mode_) {
    return; /* 模式未改变直接返回 */
  }

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < this->mixer_.len_; i++) {
    this->actuator_[i]->Reset();
  }
  this->mode_ = mode;
}

template class Module::Radar<Device::RMMotor, Device::RMMotor::Param>;
