/**
 * @file chassis.h
 * @author Qu Shen (503578404@qq.com)
 * @brief 底盘模组
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#include "comp_filter.hpp"
#include "comp_mixer.hpp"
#include "comp_pid.hpp"
#include "dev_actuator.hpp"
#include "dev_cap.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"
#include "dev_tof.hpp"

namespace Module {
class Chassis {
 public:
  /* 底盘参数的结构体，包含所有初始化用的参数，通常是const，存好几组 */
  typedef struct {
    Component::Mixer::Mode type; /* 底盘类型，底盘的机械设计和轮子选型 */

    Component::PID::Param follow_pid_param; /* 跟随云台PID的参数 */

    Device::SpeedActuator::Param actuator_param[4];

    Device::RMMotor::Param motor_param[4];
  } Param;

  typedef struct {
    Device::Referee::Status status;
    float chassis_power_limit;
    float chassis_pwr_buff;
    float chassis_watt;
  } RefForChassis;

  Chassis(Param &param, float control_freq);

  void UpdateFeedback();

  void Control();

  void SetMode(Component::CMD::ChassisMode mode);

  void PackOutput();

  void PackUI();

  void PraseRef(Device::Referee::Data &ref);

  float CalcWz(const float lo, const float hi);

  Param param_;

  float dt_;

  uint32_t last_wakeup_;

  uint32_t now_;

  ui_chassis_t ui_;

  Component::CMD::ChassisCMD cmd_;

  RefForChassis ref_;

  Component::CMD::ChassisMode mode_;

  Device::SpeedActuator *actuator_[4];

  Device::RMMotor *motor_[4];

  /* 底盘设计 */
  Component::Mixer mixer_;

  Component::Type::MoveVector move_vec_; /* 底盘实际的运动向量 */

  float gimbal_yaw_offset;

  float wz_dir_mult_; /* 小陀螺模式旋转方向乘数 */
  float vy_dir_mult_; /* scan模式移动方向乘数 */

  /* PID计算的目标值 */
  struct {
    float *motor_rotational_speed; /* 电机转速的动态数组，单位：RPM */
  } setpoint;

  /* 反馈控制用的PID */

  Component::PID follow_pid_; /* 跟随云台用的PID */

  Device::Cap::Output cap_control_;

  Device::Cap::Feedback cap_fb_;

#if RB_SENTRY
  Device::Tof::Feedback tof_fb_[Device::Tof::DEV_TOF_SENSOR_NUMBER];
#endif

  System::Thread thread_;
};
}  // namespace Module
