#pragma once

#include "comp_actuator.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_cap.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"

namespace Module {
template <typename Motor, typename MotorParam>
class Balance {
 public:
  /* 底盘运行模式 */
  typedef enum {
    Relax, /* 放松模式，电机不输出。一般情况底盘初始化之后的模式 */
    FollowGimbal, /* 通过闭环控制使车头方向跟随云台 */
    Rotor, /* 小陀螺模式，通过闭环控制使底盘不停旋转 */
  } Mode;

  typedef enum {
    ChangeModeRelax,
    ChangeModeFollow,
    ChangeModeRotor,
  } ChassisEvent;

  typedef enum {
    Left,
    Right,
    WheelNum,
  } Wheel;

  typedef struct {
    float forward_speed;
  } Feedback;

  typedef struct {
    struct {
      float speed;           /* 速度环 */
      float balance;         /* 直立环 */
      float angle[WheelNum]; /* 角度环 */
    } wheel_speed;

    struct {
      float g_center; /* 重心角度 */
      float g_comp;   /* 加速补偿 */
    } angle;
  } Setpoint;

  /* 底盘参数的结构体，包含所有初始Component化用的参数，通常是const，存好几组 */
  typedef struct {
    float init_g_center;

    Component::PID::Param follow_pid_param; /* 跟随云台PID的参数 */

    Component::PID::Param comp_pid_param;

    const std::vector<Component::CMD::EventMapItem> event_map;

    Component::SpeedActuator::Param wheel_param[WheelNum];

    Component::PID::Param eulr_param;

    Component::PID::Param gyro_param;

    Component::PID::Param speed_param;

    float center_filter_cutoff_freq;

    MotorParam motor_param[WheelNum];
  } Param;

  Balance(Param &param, float control_freq);

  void UpdateFeedback();

  void Control();

  void SetMode(Mode mode);

  void PackOutput();

  Param param_;

  float dt_;

  float last_wakeup_;

  float now_;

  Mode mode_ = Relax;

  Component::SpeedActuator *wheel_actr_[WheelNum];

  Component::PID eulr_pid_;

  Component::PID gyro_pid_;

  Component::PID speed_pid_;

  Device::BaseMotor *motor_[WheelNum];

  Component::Type::MoveVector move_vec_; /* 底盘实际的运动向量 */

  Component::Type::Eulr eulr_;
  Component::Type::Vector3 gyro_;

  float wz_dir_mult_; /* 小陀螺模式旋转方向乘数 */
  float vy_dir_mult_; /* scan模式移动方向乘数 */

  /* PID计算的输出值 */
  float motor_out[WheelNum];

  Feedback feeback_;

  Setpoint setpoint_;

  Component::PID follow_pid_; /* 跟随云台用的PID */

  Component::PID comp_pid_;

  Component::LowPassFilter2p center_filter_;

  System::Semaphore ctrl_lock_;

  Component::CMD::ChassisCMD cmd_;

  System::Thread thread_;

  Message::Topic<float> speed_err_ = Message::Topic<float>("chassis_speed_err");
};

typedef Balance<Device::RMMotor, Device::RMMotor::Param> RMBalance;
}  // namespace Module
