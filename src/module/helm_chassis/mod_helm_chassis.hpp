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
    Component::PID::Param follow_pid_param; /* 跟随云台PID的参数 */

    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    std::array<Component::PosActuator::Param, 4> change_act_param;
    std::array<Component::SpeedActuator::Param, 4> actuator_param;

    std::array<MotorParam, 4> change_motor_param;
    std::array<MotorParam, 4> motor_param;
  } Param;

  typedef struct {
    Device::Referee::Status status;
    float chassis_power_limit;
    float chassis_pwr_buff;
    float chassis_watt;
  } RefForChassis;

  HelmChassis(Param &param, float control_freq);

  void UpdateFeedback();

  void Control();

  void SetMode(Mode mode);
  void PosBack();

  void PackOutput();

  void PraseRef();

  static void DrawUIStatic(HelmChassis<Motor, MotorParam> *chassis);

  static void DrawUIDynamic(HelmChassis<Motor, MotorParam> *chassis);

  float CalcWz(const float LO, const float HI);

 private:
  Param param_;

  float dt_;

  float last_wakeup_;

  float now_;

  float wheel_speed_;
  float r_spin_;    /* 旋转量值*/
  float percentage; /* 超级电容剩余百分比 */

  std::array<float, 4> pos_out_;
  std::array<float, 4> out_;

  float yaw_;

  double r[4][2] = /* 不知道干什么用的 */
      {{-cos(M_2PI / 8), -sin(M_2PI / 8)},
       {+cos(M_2PI / 8), -sin(M_2PI / 8)},
       {+cos(M_2PI / 8), +sin(M_2PI / 8)},
       {-cos(M_2PI / 8), +sin(M_2PI / 8)}};
  float v[4][2];              /* 速度x,y矢量分解数组 */
  float real_v_[4];           /* 四速度轮实际转速数组 */
  float gyro_angle_[4];       /* 四舵轮旋转角度数组 */
  float last_angle_[4];       /* 上一次舵轮角度数组 */
  float a_[4] = {1, 1, 1, 1}; /* 不知道干什么用的 */

  RefForChassis ref_;

  Mode mode_ = RELAX;

  Device::Cap::Info cap_;

  std::array<Component::PosActuator *, 4> change_actr_;
  std::array<Component::SpeedActuator *, 4> actuator_;

  std::array<Device::BaseMotor *, 4> change_motor_;
  std::array<Device::BaseMotor *, 4> motor_;

  /* 底盘设计 */

  Component::Type::MoveVector move_vec_; /* 底盘实际的运动向量    */

  float wz_dir_mult_; /* 小陀螺模式旋转方向乘数 */

  /* PID计算的目标值 */
  struct {
    float *motor_rotational_speed; /* 电机转速的动态数组，单位：RPM */
    std::array<float, 4> wheel_pos;
  } setpoint_;

  /* 反馈控制用的PID */

  Component::PID follow_pid_; /* 跟随云台用的PID */

  System::Thread thread_;

  System::Semaphore ctrl_lock_;
  Device::Referee::Data raw_ref_;

  std::array<Component::Type::CycleValue, 4> wheel_pos_; /* 不知道写了有啥用 */

  Component::CMD::ChassisCMD cmd_;

  Component::UI::String string_;

  Component::UI::Line line_;

  Component::UI::Rectangle rectange_;
};

typedef HelmChassis<Device::RMMotor, Device::RMMotor::Param> RMChassis;
}  // namespace Module
