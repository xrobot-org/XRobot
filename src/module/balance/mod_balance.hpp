#pragma once

#include <module.hpp>

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"

namespace Module {
template <typename Motor, typename MotorParam>
class Balance {
 public:
  /* 底盘运行模式 */
  typedef enum {
    RELAX, /* 放松模式，电机不输出。一般情况底盘初始化之后的模式 */
    FOLLOW_GIMBAL, /* 通过闭环控制使车头方向跟随云台 */
    ROTOR, /* 小陀螺模式，通过闭环控制使底盘不停旋转 */
  } Mode;

  typedef enum {
    SET_MODE_RELAX,
    SET_MODE_FOLLOW,
    SET_MODE_ROTOR,
  } ChassisEvent;

  typedef enum {
    LEFT_WHEEL,
    RIGHT_WHEEL,
    WHEEL_NUM,
  } Wheel;

  typedef struct {
    float forward_speed;
  } Feedback;

  typedef struct {
    struct {
      float speed;                        /* 速度环 */
      float balance;                      /* 直立环 */
      std::array<float, WHEEL_NUM> angle; /* 角度环 */
    } wheel_speed;

    struct {
      float g_comp; /* 加速补偿 */
      Component::Type::CycleValue yaw;
    } angle;
  } Setpoint;

  /* 底盘参数的结构体，包含所有初始Component化用的参数，通常是const，存好几组 */
  typedef struct {
    float init_g_center;

    Component::PID::Param follow_pid_param; /* 跟随云台PID的参数 */

    Component::PID::Param comp_pid_param;

    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    std::array<Component::SpeedActuator::Param, WHEEL_NUM> wheel_param;

    Component::PID::Param eulr_param;

    Component::PID::Param gyro_param;

    Component::PID::Param speed_param;

    float center_filter_cutoff_freq;

    std::array<MotorParam, WHEEL_NUM> motor_param;
  } Param;

  Balance(Param &param, float control_freq);

  void UpdateFeedback();

  void Control();

  void SetMode(Mode mode);

  void PackOutput();

 private:
  Param param_;

  float dt_;

  float last_wakeup_;

  float now_;

  Mode mode_ = RELAX;

  std::array<Component::SpeedActuator *, WHEEL_NUM> wheel_actr_;

  Component::PID eulr_pid_;

  Component::PID gyro_pid_;

  Component::PID speed_pid_;

  std::array<Device::BaseMotor *, WHEEL_NUM> motor_;

  Component::Type::MoveVector move_vec_; /* 底盘实际的运动向量 */

  Component::Type::Eulr eulr_;
  Component::Type::Vector3 gyro_;

  float wz_dir_mult_; /* 小陀螺模式旋转方向乘数 */
  float vy_dir_mult_; /* scan模式移动方向乘数 */

  /* PID计算的输出值 */
  std::array<float, WHEEL_NUM> motor_out_;

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
