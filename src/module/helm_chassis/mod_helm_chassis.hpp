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
#include <cstdlib>
#include <module.hpp>
#include <vector>

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_mixer.hpp"
#include "comp_pid.hpp"
#include "dev_cap.hpp"
#include "dev_motor.hpp"
#include "dev_referee.hpp"
#include "dev_rm_motor.hpp"
#define PI_ (M_2PI)

namespace Module {
template <typename Motor, typename MotorParam>
class HelmChassis {
 public:
  /* 底盘运行模式 */
  typedef enum {
    RELAX, /* 放松模式，电机不输出。一般情况底盘初始化之后的模式 */
    BREAK, /* 刹车模式，电机闭环控制保持静止。用于机器人停止状态 */
    CHASSIS_FOLLOW_GIMBAL, /* 通过闭环控制使车头方向跟随云台 */
    CHASSIS_6020_FOLLOW_GIMBAL, /*6020电机直接跟随车头方向*/
    ROTOR, /* 小陀螺模式，通过闭环控制使底盘不停旋转 */
    INDENPENDENT, /* 独立模式。底盘运行不受云台影响 */
  } Mode;

  typedef enum {
    SET_MODE_RELAX,
    SET_MODE_CHASSIS_FOLLOW,
    SET_MODE_6020_FOLLOW,
    SET_MODE_ROTOR,
    SET_MODE_INDENPENDENT,
  } ChassisEvent;

  /* 底盘参数的结构体，包含所有初始Component化用的参数，通常是const，存好几组 */
  typedef struct Param {
    float toque_coefficient_3508;
    float speed_2_coefficient_3508;
    float out_2_coefficient_3508;
    float constant_3508;
    Component::PID::Param follow_pid_param{}; /* 跟随云台PID的参数 */
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    std::array<Component::PosActuator::Param, 4> pos_param{};
    std::array<Component::SpeedActuator::Param, 4> speed_param{};
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

  bool LimitChassisOutPower(float power_limit, float *motor3508_out,
                            float *speed_3508, uint32_t len);
  bool LimitChassisOutput(float power_limit, float *motor_out, float *speed,
                          uint32_t len);
  // float Calculate6020Power(float *motor_out, float *speed, uint32_t len);

  uint16_t MAXSPEEDGET(float power_limit);

  void SetMode(Mode mode);

  static void DrawUIStatic(HelmChassis<Motor, MotorParam> *chassis);

  static void DrawUIDynamic(HelmChassis<Motor, MotorParam> *chassis);

  float CalcWz(const float LO, const float HI);

  int Getrandom();  //随机数

  double GetTorqueLength(float angle_a, double angle_b, int selection);

  float GetRelateAngle();  //相对偏航角

 private:
  Param param_;

  float dt_;

  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;
  float yaw_;
  float pit_;
  double alpha_;
  float pos_motor_value_[4];
  float speed_motor_value_[4];
  float pos_err_value_[4];
  float wz_test_;
  float pos_motor_sum_out_;
  float eulr_yaw1_;
  int random_;
  float deviation_angle_value_;
  float test_angle01_;
  float test_angle02_;
  double tan_pit_;
  std::vector<std::string> string_vector1_{
      "Chassis_pos_RightFront", "Chassis_pos_LeftFront", "Chassis_pos_LeftBack",
      "Chassis_pos_RightBack"};

  std::vector<std::string> string_vector2_{
      "Chassis_speed_RightFront", "Chassis_speed_LeftFront",
      "Chassis_speed_LeftBack", "Chassis_speed_RightBack"};

  float max_motor_rotational_speed_;

  Device::Cap::Info cap_;

  Device::Referee::Data raw_ref_;

  float speed_motor_feedback_[4];
  float pos_motor_feedback_[4];
  float forward_coefficient_[4];
  float l_data_[4];
  float power_;
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
  struct {
    float speed_motor_out[4];
    float motor6020_out[4];
  } out_;
  Component::Type::CycleValue main_direct_ = 0.0f;

  float direct_offset_ = 0.0f;

  float state_num_;

  Component::PID follow_pid_;

  System::Thread thread_;

  System::Semaphore ctrl_lock_;

  Component::UI::String string_;

  Component::UI::Line line_;

  Component::UI::Rectangle rectange_;

  Component::CMD::ChassisCMD cmd_;

  std::array<float, 4> speed_motor_out_;

  std::array<float, 4> pos_motor_out_;

  Component::Type::Eulr eulr_;
  Component::Type::Quaternion quat_;
  Component::Type::Vector3 gyro_;
};

typedef HelmChassis<Device::RMMotor, Device::RMMotor::Param> RMHelmChassis;
}  // namespace Module
