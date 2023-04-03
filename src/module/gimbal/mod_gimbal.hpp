/*
 * 云台模组
 */

#pragma once

#include <comp_type.hpp>
#include <module.hpp>

#include "comp_actuator.hpp"
#include "comp_cf.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_ahrs.hpp"
#include "dev_bmi088.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"

namespace Module {
class Gimbal {
 public:
  /* 云台运行模式 */
  typedef enum {
    RELAX, /* 放松模式，电机不输出。一般情况云台初始化之后的模式 */
    ABSOLUTE, /* 绝对坐标系控制，控制在空间内的绝对姿态 */
  } Mode;

  enum {
    GIMBAL_CTRL_YAW_OMEGA_IDX = 0, /* Yaw轴控制的角速度环控制器的索引值 */
    GIMBAL_CTRL_YAW_ANGLE_IDX, /* Yaw轴控制的角度环控制器的索引值 */
    GIMBAL_CTRL_PIT_OMEGA_IDX, /* Pitch轴控制的角速度环控制器的索引值 */
    GIMBAL_CTRL_PIT_ANGLE_IDX, /* Pitch轴控制的角度环控制器的索引值 */
    GIMBAL_CTRL_NUM,           /* 总共的控制器数量 */
  };

  typedef enum {
    SET_MODE_RELAX,
    SET_MODE_ABSOLUTE,
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
      Component::Type::CycleValue pitch_max;
      Component::Type::CycleValue pitch_min;
    } limit;

    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

  } Param;

  Gimbal(Param &param, float control_freq);

  void UpdateFeedback();

  void Control();

  void SetMode(Mode mode);

  static void DrawUIStatic(Gimbal *gimbal);

  static void DrawUIDynamic(Gimbal *gimbal);

 private:
  float last_wakeup_;

  float now_;

  float dt_;

  Param param_;

  Gimbal::Mode mode_ = RELAX; /* 云台模式 */

  struct {
    Component::Type::Eulr eulr_; /* 表示云台姿态的欧拉角 */
  } setpoint_;

  Component::SecOrderFunction st_; /* YAW自整定参数 */

  Component::PosActuator yaw_actuator_;
  Component::PosActuator pit_actuator_;

  Device::RMMotor yaw_motor_;
  Device::RMMotor pit_motor_;

  System::Thread thread_;

  System::Semaphore ctrl_lock_;

  Message::Topic<float> yaw_tp_ = Message::Topic<float>("chassis_yaw");

  float yaw_;

  Component::UI::String string_;

  Component::UI::Rectangle rectangle_;

  Component::Type::Eulr eulr_;
  Component::Type::Vector3 gyro_;
  Component::CMD::GimbalCMD cmd_;
};
}  // namespace Module
