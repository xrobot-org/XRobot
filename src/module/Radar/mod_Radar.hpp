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

#include <module.hpp>

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_mixer.hpp"
#include "comp_pid.hpp"
#include "dev_motor.hpp"
#include "dev_rm_motor.hpp"

namespace Module {
template <typename Motor, typename MotorParam>
class Radar {
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
  typedef struct Param {
    float toque_coefficient_;
    float speed_2_coefficient_;
    float out_2_coefficient_;
    float constant_;

    Component::Mixer::Mode type =
        Component::Mixer::MECANUM; /* 底盘类型，底盘的机械设计和轮子选型 */

    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    std::array<Component::SpeedActuator::Param, 4> actuator_param{};

    std::array<MotorParam, 4> motor_param;
    //float (*get_speed)(float);
  } Param;

  Radar(Param &param, float control_freq);

  void UpdateFeedback();

  void Control();

  void SetMode(Mode mode);

  uint16_t MAXSPEEDGET(float power_limit);

 private:
  Param param_;

  float max_motor_rotational_speed_ = 0.0f;

  float dt_ = 0.0f;

  float chassis_current_;

  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;


  Mode mode_ = RELAX;


  std::array<Component::SpeedActuator *, 4> actuator_;

  std::array<Device::BaseMotor *, 4> motor_;

  /* 底盘设计 */
  Component::Mixer mixer_;

  Component::Type::MoveVector move_vec_; /* 底盘实际的运动向量 */

  float wz_dir_mult_; /* 小陀螺模式旋转方向乘数 */

  /* PID计算的目标值 */
  struct {
    float *motor_rotational_speed; /* 电机转速的动态数组，单位：RPM */
  } setpoint_;

  float motor_feedback_[4];

  struct {
    float motor_out[4];
  } out_;


  System::Thread thread_;

  System::Semaphore ctrl_lock_;

  Component::CMD::ChassisCMD cmd_;

};

typedef Radar<Device::RMMotor, Device::RMMotor::Param> Radar_car;
}  // namespace Module
