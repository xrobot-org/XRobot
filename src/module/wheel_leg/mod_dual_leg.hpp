#pragma once

#include <module.hpp>

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "comp_filter.hpp"
#include "comp_mixer.hpp"
#include "comp_pid.hpp"
#include "comp_triangle.hpp"
#include "dev_mit_motor.hpp"

/*          L1              LEFT   L4  RIGHT  */
/* M_FRONT ☉---☉ M_BACK       ☉----------☉    */
/*        /     \             |          |    */
/*       /       \L2          |          |    */
/*      /         \           |          |    */
/*     ◉           ◉          ◉          ◉    */
/*      \         /           |          |    */
/*       \  ___  /L3          |          |    */
/*        /     \            ---        ---   */
/*       |   ◉   |           | |        | |   */
/*        \ ___ /            ---        ---   */
namespace Module {
class WheelLeg {
 public:
  typedef enum { LEG_LEFT, LEG_RIGHT, LEG_NUM } Leg;
  typedef enum { LEG_FRONT, LEG_BACK, LEG_MOTOR_NUM } LegMotor;

  typedef enum {
    RELAX, /* 放松模式，电机不输出 */
    BREAK, /* 电机保持当前角度 */
    SQUAT, /* 稳定模式，以较低高度补偿角度 */
    JUMP,  /* 短时间内移动至最高高度后，切换至稳定模式 */
  } Mode;

  typedef enum {
    SET_MODE_RELAX,
    SET_MODE_BREAK,
    SET_MODE_SQUAT,
    SET_MODE_JUMP,
  } ChassisEvent;

  typedef struct {
    float l1;
    float l2;
    float l3;
    float l4;

    struct {
      float high_max;
      float high_min;
    } limit;

    float leg_max_angle;

    /* 电机关节指向底盘正前方的角度 */
    std::array<Component::Type::CycleValue, LEG_NUM * LEG_MOTOR_NUM> motor_zero;

    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    std::array<Component::PosActuator::Param, LEG_NUM * LEG_MOTOR_NUM> leg_actr;

    std::array<Device::MitMotor::Param, LEG_NUM * LEG_MOTOR_NUM> leg_motor;

  } Param;

  typedef struct {
    std::array<Component::Type::CycleValue, LEG_MOTOR_NUM> motor_angle;
    Component::Type::Line diagonal;
    Component::Type::Polar2 whell_polar;
    Component::Type::Position2 whell_pos;
  } Feedback;

  typedef struct {
    Component::Type::Position2 whell_pos;
    std::array<float, LEG_MOTOR_NUM> motor_angle;
  } Setpoint;

  WheelLeg(Param& param, float sample_freq);

  void UpdateFeedback();

  void Control();

  void SetMode(Mode mode);

 private:
  Param param_;

  float dt_ = 0.0f;

  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;

  std::array<Component::PosActuator*, LEG_NUM * LEG_MOTOR_NUM> leg_actuator_;

  std::array<Device::MitMotor*, LEG_NUM * LEG_MOTOR_NUM> leg_motor_;

  Component::Type::Eulr eulr_;

  Component::Type::Vector3 gyro_;

  std::array<Setpoint, LEG_NUM> setpoint_;

  std::array<Feedback, LEG_NUM> feedback_;

  Mode mode_ = RELAX;

  Message::Topic<Component::Type::Polar2> wheel_polor_;

  System::Semaphore ctrl_lock_;

  System::Thread thread_;
};
}  // namespace Module
