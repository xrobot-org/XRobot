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

#include "mod_balance.hpp"

#include <stdlib.h>

#include "dev_can.hpp"
#include "dev_cap.hpp"
#include "dev_dr16.hpp"
#include "dev_tof.hpp"

#define ROTOR_WZ_MIN 0.6f /* 小陀螺旋转位移下界 */
#define ROTOR_WZ_MAX 0.8f /* 小陀螺旋转位移上界 */

#define ROTOR_OMEGA 0.0015f /* 小陀螺转动频率 */

#define MOTOR_MAX_ROTATIONAL_SPEED 7000.0f /* 电机的最大转速 */

using namespace Module;

template <typename Motor, typename MotorParam>
Balance<Motor, MotorParam>::Balance(Param& param, float control_freq)
    : param_(param),
      mode_(Balance::Relax),
      balance_actr_(param.balance_param, control_freq),
      speed_actr_(param.speed_param, control_freq),
      follow_pid_(param.follow_pid_param, control_freq),
      comp_pid_(param.comp_pid_param, control_freq),
      center_filter_(control_freq, param.center_filter_cutoff_freq),
      ctrl_lock_(true) {
  memset(&(this->cmd_), 0, sizeof(this->cmd_));

  this->setpoint_.angle.g_center = 0.13f;

  for (uint8_t i = 0; i < WheelNum; i++) {
    this->wheel_actr_[i] = (Component::SpeedActuator*)System::Memory::Malloc(
        sizeof(Component::SpeedActuator));
    new (this->wheel_actr_[i])
        Component::SpeedActuator(param.wheel_param[i], control_freq);

    this->motor_[i] = (Motor*)System::Memory::Malloc(sizeof(Motor));
    new (this->motor_[i])
        Motor(param.motor_param[i],
              (std::string("chassis_") + std::to_string(i)).c_str());
  }

  auto event_callback = [](uint32_t event, void* arg) {
    Balance* chassis = static_cast<Balance*>(arg);

    chassis->ctrl_lock_.Take(UINT32_MAX);

    switch (event) {
      case ChangeModeRelax:
        chassis->SetMode(Relax);
        break;
      case ChangeModeFollow:
        chassis->SetMode(FollowGimbal);
        break;
      case ChangeModeRotor:
        chassis->SetMode(Rotor);
        break;
      default:
        break;
    }

    chassis->ctrl_lock_.Give();
  };

  Component::CMD::RegisterEvent(event_callback, this, this->param_.event_map);

  auto chassis_thread = [](void* arg) {
    Balance* chassis = (Balance*)arg;

    DECLARE_SUBER(cmd_, chassis->cmd_, "cmd_chassis");
    DECLARE_SUBER(eulr_, chassis->eulr_, "imu_eulr");
    DECLARE_SUBER(gyro_, chassis->gyro_, "imu_gyro");

    while (1) {
      /* 读取控制指令、电容、裁判系统、电机反馈 */
      cmd_.DumpData();
      eulr_.DumpData();
      gyro_.DumpData();

      /* 更新反馈值 */
      chassis->ctrl_lock_.Take(UINT32_MAX);
      chassis->UpdateFeedback();
      chassis->Control();
      chassis->ctrl_lock_.Give();

      /* 运行结束，等待下一次唤醒 */
      chassis->thread_.Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, chassis_thread, 384, System::Thread::Medium,
                 this);
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::UpdateFeedback() {
  /* 将CAN中的反馈数据写入到feedback中 */
  for (size_t i = 0; i < WheelNum; i++) {
    this->motor_[i]->Update();
  }

  this->feeback_.forward_speed =
      (this->motor_[Right]->GetSpeed() - this->motor_[Left]->GetSpeed()) /
      2.0f / MOTOR_MAX_ROTATIONAL_SPEED;
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::Control() {
  this->now_ = System::Thread::GetTick();

  this->dt_ = (float)(this->now_ - this->last_wakeup_) / 1000.0f;
  this->last_wakeup_ = this->now_;

  /* ctrl_vec -> move_vec 控制向量和真实的移动向量之间有一个换算关系 */
  /* 计算vx、vy */
  switch (this->mode_) {
    case Balance::Relax:
      this->move_vec_.vx = 0.0f;
      this->move_vec_.vy = 0.0f;
      break;
    case Balance::Rotor:
    case Balance::FollowGimbal:
      this->move_vec_.vx = this->cmd_.x;
      this->move_vec_.vy = this->cmd_.y;
      break;

    default:
      break;
  }

  /* 计算wz */
  switch (this->mode_) {
    case Balance::Relax:
    case Balance::FollowGimbal:
    case Balance::Rotor:
      this->move_vec_.wz = 0;
      break;

    default:
      ASSERT(false);
      return;
  }

  /* 根据底盘模式计算输出值 */
  switch (this->mode_) {
    case Balance::FollowGimbal:
    case Balance::Rotor: {
      /* 速度环 */
      this->setpoint_.wheel_speed.speed = -this->speed_actr_.Calculation(
          this->move_vec_.vy, this->feeback_.forward_speed, this->dt_);

      /* 加减速时保证稳定姿态 */
      this->setpoint_.angle.g_comp = this->comp_pid_.Calculate(
          this->move_vec_.vy, this->feeback_.forward_speed, this->dt_);

      /* 速度误差积分估计重心 */
      this->setpoint_.angle.g_center += this->center_filter_.Apply(
          (this->feeback_.forward_speed - this->move_vec_.vy) * this->dt_ *
          0.3);

      clampf(&(this->setpoint_.angle.g_center), -M_PI / 6.0f, M_PI / 6.0f);

      /* 直立环 */
      for (uint8_t i = 0; i < WheelNum; i++) {
        this->setpoint_.wheel_speed.balance[i] =
            this->balance_actr_.Calculation(
                this->setpoint_.angle.g_center - this->setpoint_.angle.g_comp,
                this->gyro_.x, this->eulr_.pit, this->dt_);
      }

      /* 角度环 */
      this->setpoint_.wheel_speed.angle[Left] = -this->move_vec_.vx;
      this->setpoint_.wheel_speed.angle[Right] = this->move_vec_.vx;

      /* 轮子速度控制 */
      for (uint8_t i = 0; i < WheelNum; i++) {
        float speed_sp = this->setpoint_.wheel_speed.speed +
                         this->setpoint_.wheel_speed.balance[i] +
                         this->setpoint_.wheel_speed.angle[i];

        clampf(&speed_sp, -1.0f, 1.0f);

        if (i == Left) {
          speed_sp = -speed_sp;
        }

        this->motor_out[i] = this->wheel_actr_[i]->Calculation(
            speed_sp * MOTOR_MAX_ROTATIONAL_SPEED, this->motor_[i]->GetSpeed(),
            this->dt_);
      }

      /* 电机输出 */
      for (uint8_t i = 0; i < WheelNum; i++) {
        this->motor_[i]->Control(this->motor_out[i]);
      }
      break;
    }

    case Balance::Relax: /* 放松模式,不输出 */
      for (size_t i = 0; i < WheelNum; i++) {
        this->motor_[i]->Relax();
      }
      break;
    default:
      ASSERT(false);
      return;
  }
}

template <typename Motor, typename MotorParam>
void Balance<Motor, MotorParam>::SetMode(Balance::Mode mode) {
  if (mode == this->mode_) return; /* 模式未改变直接返回 */

  /* 切换模式后重置PID和滤波器 */
  for (size_t i = 0; i < WheelNum; i++) {
    this->wheel_actr_[i]->Reset();
  }

  this->balance_actr_.Reset();
  this->mode_ = mode;
}

template class Balance<Device::RMMotor, Device::RMMotor::Param>;
