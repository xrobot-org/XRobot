/*
 * 云台模组
 */

#pragma once

#include "comp_actuator.hpp"
#include "comp_ahrs.hpp"
#include "comp_cf.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_bmi088.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"

namespace Module {
class Gimbal {
 public:
  /* 云台运行模式 */
  typedef enum {
    Relax, /* 放松模式，电机不输出。一般情况云台初始化之后的模式 */
    Absolute, /* 绝对坐标系控制，控制在空间内的绝对姿态 */
  } Mode;

  enum {
    GIMBAL_CTRL_YAW_OMEGA_IDX = 0, /* Yaw轴控制的角速度环控制器的索引值 */
    GIMBAL_CTRL_YAW_ANGLE_IDX, /* Yaw轴控制的角度环控制器的索引值 */
    GIMBAL_CTRL_PIT_OMEGA_IDX, /* Pitch轴控制的角速度环控制器的索引值 */
    GIMBAL_CTRL_PIT_ANGLE_IDX, /* Pitch轴控制的角度环控制器的索引值 */
    GIMBAL_CTRL_NUM,           /* 总共的控制器数量 */
  };

  typedef enum {
    SetModeRelax,
    SetModeAbsolute,
  } GimbalEvent;

  typedef struct {
    Component::SecOrderFunction::Param ff; /* PITCH前馈 */
    Component::SecOrderFunction::Param st; /* YAW自整定参数 */

    Component::PosActuator::Param yaw_actr;
    Component::PosActuator::Param pit_actr;

    Device::RMMotor::Param yaw_motor;
    Device::RMMotor::Param pit_motor;

    Component::Type::Eulr mech_zero;

    struct {
      float pitch_max;
      float pitch_min;
    } limit;

    const std::vector<Component::CMD::EventMapItem> event_map;

  } Param;

  Gimbal(Param &param, float control_freq);

  void UpdateFeedback();

  void Control();

  void SetMode(Mode mode);

  uint32_t last_wakeup_;

  float now_;

  float dt_;

  Param param_;

  Gimbal::Mode mode_ = Relax; /* 云台模式 */

  struct {
    Component::Type::Eulr eulr_; /* 表示云台姿态的欧拉角 */
  } setpoint;

  Component::SecOrderFunction st_; /* YAW自整定参数 */

  Component::PosActuator yaw_actuator_;
  Component::PosActuator pit_actuator_;

  Device::RMMotor yaw_motor_;
  Device::RMMotor pit_motor_;

  System::Thread thread_;

  System::Semaphore ctrl_lock_;

  Message::Topic<float> yaw_tp_ = Message::Topic<float>("gimbal_yaw_offset");

  float yaw_;

  Component::Type::Eulr eulr_;
  Component::Type::Vector3 gyro_;
  Component::CMD::GimbalCMD cmd_;
};
}  // namespace Module
