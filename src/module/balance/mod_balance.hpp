#pragma once

#include <array>
#include <comp_type.hpp>
#include <module.hpp>

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_pid.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"
#include "dev_rmd_motor.hpp"

namespace Module {
template <typename Motor, typename MotorParam>
class Balance {
 public:
  /* 底盘运行模式 */
  typedef enum {
    RELAX, /* 放松模式，电机不输出。一般情况底盘初始化之后的模式 */
    FOLLOW_GIMBAL, /* 通过闭环控制使车头方向跟随云台 */
    INDENPENDENT,  /* 独立模式。底盘运行不受云台影响 */
    ROTOR, /* 小陀螺模式，通过闭环控制使底盘不停旋转 */
  } Mode;

  typedef enum {
    MOVING,     /* 运动 */
    STATIONARY, /* 静止 */
    SLIP,       /* 打滑 */
    ROLLOVER    /* 翻倒 */
  } Status;

  typedef enum {
    SET_MODE_RELAX,
    SET_MODE_FOLLOW,
    SET_MODE_INDENPENDENT,
    SET_MODE_ROTOR,
  } ChassisEvent;

  typedef enum {
    LEFT_WHEEL,
    RIGHT_WHEEL,
    WHEEL_NUM,
  } Wheel;

  typedef enum {
    CTRL_CH_DISPLACEMENT,
    CTRL_CH_FORWARD_SPEED,
    CTRL_CH_PITCH_ANGLE,
    CTRL_CH_GYRO_X,
    CTRL_CH_YAW_ANGLE,
    CTRL_CH_GYRO_Z,
    CTRL_CH_NUM
  } ControlChannel;

  typedef std::array<float, CTRL_CH_NUM> Feedback;

  typedef std::array<float, CTRL_CH_NUM> Setpoint;

  typedef std::array<float, CTRL_CH_NUM> Output;

  typedef struct {
    Component::Type::CycleValue init_g_center;

    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    float speed_filter_cutoff_freq;

    std::array<MotorParam, WHEEL_NUM> motor_param;

    std::array<Component::PID::Param, CTRL_CH_NUM> pid_param;
  } Param;

  Balance(Param &param, float control_freq);

  void UpdateFeedback();

  void UpdateStatus();

  void Control();

  void SetMode(Mode mode);

  bool SlipDetect();

  bool RolloverDetect();

 private:
  Param param_;

  float dt_;

  float last_wakeup_;

  float now_;

  Mode mode_ = RELAX;

  Status status_ = STATIONARY;

  uint8_t pid_enable_ = 0;

  std::array<Component::PID *, CTRL_CH_NUM> pid_;
  std::array<Device::BaseMotor *, WHEEL_NUM> motor_;

  Feedback feeback_;

  Setpoint setpoint_;

  Output output_;

  Component::Type::MoveVector move_vec_; /* 底盘实际的运动向量 */

  Component::Type::Eulr eulr_;
  Component::Type::Vector3 gyro_;

  float wz_dir_mult_; /* 小陀螺模式旋转方向乘数 */

  /* PID计算的输出值 */
  std::array<float, WHEEL_NUM> motor_out_;

  Component::LowPassFilter2p speed_filter_;

  System::Semaphore ctrl_lock_;

  Component::CMD::ChassisCMD cmd_;

  System::Thread thread_;

  Message::Topic<float> speed_err_ = Message::Topic<float>("chassis_speed_err");
};

typedef Balance<Device::RMMotor, Device::RMMotor::Param> RMBalance;
typedef Balance<Device::RMDMotor, Device::RMDMotor::Param> RMDBalance;
}  // namespace Module
