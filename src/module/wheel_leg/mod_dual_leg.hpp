#pragma once

#include "comp_actuator.hpp"
#include "comp_filter.hpp"
#include "comp_mixer.hpp"
#include "comp_pid.hpp"
#include "comp_triangle.hpp"
#include "dev_mit_motor.hpp"

/*          L1          */
/* M_FRONT ☉---☉ M_BACK */
/*        /     \       */
/*       /       \L2    */
/*      /         \     */
/*     ◉           ◉    */
/*      \         /     */
/*       \  ___  /L3    */
/*        /     \       */
/*       |   ◉   |      */
/*        \ ___ /       */
namespace Module {
class BalanceChassis {
 public:
  typedef enum { LEFT, RIGHT, LEG_NUM } Leg;
  typedef enum { FRONT, BACK, LEG_MOTOR_NUM } LegMotor;

  typedef enum {
    Relax, /* 放松模式，电机不输出 */
    Break, /* 电机保持当前角度 */
    Squat, /* 稳定模式，以较低高度补偿角度 */
    Jump,  /* 短时间内移动至最高高度后，切换至稳定模式 */
  } Mode;

  typedef struct {
    float l1;
    float l2;
    float l3;

    float leg_max_angle;

    float mech_zero[LEG_NUM * LEG_MOTOR_NUM];

    Component::PosActuator::Param leg_actr[LEG_NUM * LEG_MOTOR_NUM];

    Device::MitMotor::Param leg_motor[LEG_NUM * LEG_MOTOR_NUM];
  } Param;

  typedef struct {
    float motor_angle[LEG_MOTOR_NUM];
    Component::Type::Line diagonal;
    Component::Type::Polar2 whell_polar;
    Component::Type::Position2 whell_pos;
  } Feedback;

  typedef struct {
    Component::Type::Position2 whell_pos;
    float motor_angle[LEG_MOTOR_NUM];
  } Setpoint;

  BalanceChassis(Param& param, float sample_freq);

  void UpdateFeedback();

  void Control();

  Param param_;

  float dt_;

  uint32_t last_wakeup_;

  uint32_t now_;

  Component::PosActuator* leg_actuator_[LEG_NUM * LEG_MOTOR_NUM];

  Device::MitMotor* leg_motor_[LEG_NUM * LEG_MOTOR_NUM];

  Setpoint setpoint_[LEG_NUM];

  Feedback feedback_[LEG_NUM];

  Mode mode_ = Relax;

  System::Thread thread_;
};
}  // namespace Module
