#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "dev_mech.hpp"
#include "dev_rm_motor.hpp"
#include "module.hpp"

namespace Module {
class Dartgimbal {
 public:
  /* 云台运行模式 */
  typedef enum {
    RELAX, /* 放松模式，电机不输出。一般情况云台初始化之后的模式 */
    ABSOLUTE, /* 绝对坐标系控制，控制在空间内的绝对姿态 */
  } Mode;
  typedef enum {
    SET_MODE_RELAX,
    SET_MODE_ABSOLUTE,
  } GimbalEvent;

  typedef struct {
    float yaw;
    float pitch;
  } Setpoint;
  typedef struct {
    float dt;
    float pos_motor;
    Component::Type::CycleValue last_pos;
  } Pi;

  typedef struct {
    Component::PosActuator::Param yaw_actr;
    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param pitch_actr;
    Device::RMMotor::Param yaw_motor;
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;
    float yaw_zero;
  } Param;
  Dartgimbal(Param &param, float control_freq);

  void UpdateFeedback();

  void Control();

 private:
  float last_wakeup_;

  float now_;

  float dt_;
  Param param_;
  Setpoint setpoint_;

  float yaw_out_;

  Component::PosActuator yaw_actr_;
  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1>
      pitch_actr_;

  Device::RMMotor yaw_motor_;

  System::Thread thread_;
};
}  // namespace Module
