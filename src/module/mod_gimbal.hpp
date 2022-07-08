/*
 * 云台模组
 */

#pragma once

#include "comp_ahrs.hpp"
#include "comp_cf.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_actuator.hpp"
#include "dev_bmi088.hpp"
#include "dev_referee.hpp"

namespace Module {
class Gimbal {
 public:
  enum {
    GIMBAL_CTRL_YAW_OMEGA_IDX = 0, /* Yaw轴控制的角速度环控制器的索引值 */
    GIMBAL_CTRL_YAW_ANGLE_IDX, /* Yaw轴控制的角度环控制器的索引值 */
    GIMBAL_CTRL_PIT_OMEGA_IDX, /* Pitch轴控制的角速度环控制器的索引值 */
    GIMBAL_CTRL_PIT_ANGLE_IDX, /* Pitch轴控制的角度环控制器的索引值 */
    GIMBAL_CTRL_NUM,           /* 总共的控制器数量 */
  };

  enum {
    GIMBAL_ACTR_YAW_IDX = 0, /* Yaw轴动作器的索引值 */
    GIMBAL_ACTR_PIT_IDX,     /* Pitch动作器索引值 */
    GIMBAL_ACTR_NUM,         /* 总共的动作器数量 */
  };

  typedef struct {
    Component::SecOrderFunction::Param ff; /* PITCH前馈 */
    Component::SecOrderFunction::Param st; /* YAW自整定参数 */

    Device::Actuator::PosParam actuator[GIMBAL_ACTR_NUM];

    Component::Type::Eulr mech_zero;

    struct {
      float pitch_max;
      float pitch_min;
    } limit;

  } Param;

  /* 云台反馈数据的结构体，包含反馈控制用的反馈数据 */
  typedef struct {
    Component::Type::Vector3 gyro; /* IMU的陀螺仪数据 */

    /* 欧拉角 */
    Component::Type::Eulr imu; /* 由IMU计算的欧拉角 */

  } Feedback;

  Gimbal(Param &param);

  void UpdateFeedback();

  void Control();

  void PackUI();

  void SetMode(Component::CMD::GimbalMode mode);

  uint32_t lask_wakeup_;

  uint32_t now_;

  float dt_;

  Param param_;

  Component::CMD::GimbalMode mode_; /* 云台模式 */

  struct {
    Component::Type::Eulr eulr_; /* 表示云台姿态的欧拉角 */
  } setpoint;

  Feedback feedback_; /* 反馈 */

  float yaw_offset_angle_;

  float scan_yaw_direction_; /* 扫描模式yaw轴旋转方向 */
  float scan_pit_direction_; /* 扫描模式pit轴旋转方向 */

  Component::SecOrderFunction ff_; /* PITCH前馈 */
  Component::SecOrderFunction st_; /* YAW自整定参数 */

  Device::Actuator *actuator_;

  Component::CMD::GimbalCMD cmd_;

  ui_gimbal_t ui_;

  System::Thread thread_;
};
}  // namespace Module
