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

#include <sys/_stdint.h>

#include <array>
#include <cmath>
#include <comp_type.hpp>
#include <module.hpp>

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_mixer.hpp"
#include "comp_pid.hpp"
#include "dev_cap.hpp"
#include "dev_motor.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"
#define PI (M_2PI)

namespace Module {
template <typename Motor, typename MotorParam>
class HelmChassis {
 public:
  /* 底盘运行模式 */
  typedef enum {
    RELAX, /* 放松模式，电机不输出。一般情况底盘初始化之后的模式 */
    BREAK, /* 刹车模式，电机闭环控制保持静止。用于机器人停止状态 */
    FOLLOW_GIMBAL, /* 通过闭环控制使车头方向跟随云台 */
    ROTOR, /* 小陀螺模式，通过闭环控制使底盘不停旋转 */
    INDENPENDENT, /* 独立模式。底盘运行不受云台影响 */
  } Mode;

  typedef enum {
    SET_MODE_RELAX,
    SET_MODE_FOLLOW,
    SET_MODE_ROTOR,
    SET_MODE_INDENPENDENT,
  } ChassisEvent;

  /* 底盘参数的结构体，包含所有初始Component化用的参数，通常是const，存好几组 */
  typedef struct {
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    std::array<Component::PosActuator::Param, 4> pos_param;
    std::array<Component::SpeedActuator::Param, 4> speed_param;
    std::array<Component::Type::CycleValue, 4> mech_zero;

    std::array<MotorParam, 4> pos_motor_param;
    std::array<MotorParam, 4> speed_motor_param;
  } Param;

  typedef struct {
    Device::Referee::Status status;
    float chassis_power_limit;
    float chassis_pwr_buff;
    float chassis_watt;
  } RefForChassis;

  HelmChassis(Param &param, float control_freq);

  void PraseRef();

  void UpdateFeedback();

  void Control();

  void SetMode(Mode mode);

  static void DrawUIStatic(HelmChassis<Motor, MotorParam> *chassis);

  static void DrawUIDynamic(HelmChassis<Motor, MotorParam> *chassis);

  float CalcWz(const float LO, const float HI);

 private:
  Param param_;

  float dt_;

  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;

  float yaw_;

  Device::Cap::Info cap_;

  Device::Referee::Data raw_ref_;

  RefForChassis ref_;

  Mode mode_ = RELAX;

  std::array<Component::PosActuator *, 4> pos_actr_;
  std::array<Component::SpeedActuator *, 4> speed_actr_;

  std::array<Device::BaseMotor *, 4> pos_motor_;
  std::array<Device::BaseMotor *, 4> speed_motor_;

  bool motor_reverse_[4];

  /* 底盘设计 */

  Component::Type::MoveVector move_vec_; /* 底盘实际的运动向量    */

  float wz_dir_mult_; /* 小陀螺模式旋转方向乘数 */

  /* PID计算的目标值 */
  struct {
    float motor_rotational_speed[4]; /* 电机转速的动态数组，单位：RPM */
    std::array<Component::Type::CycleValue, 4> wheel_pos;
  } setpoint_;

  Component::Type::CycleValue main_direct_ = 0.0f;
  float direct_offset_ = 0.0f;

  System::Thread thread_;

  System::Semaphore ctrl_lock_;

  Component::UI::String string_;

  Component::UI::Line line_;

  Component::UI::Rectangle rectange_;

  Component::CMD::ChassisCMD cmd_;

  std::array<float, 4> speed_motor_out_;

  std::array<float, 4> pos_motor_out_;
};

typedef HelmChassis<Device::RMMotor, Device::RMMotor::Param> RMHelmChassis;
}  // namespace Module
