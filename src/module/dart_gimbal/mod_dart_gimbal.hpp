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
    STABLE,
    CONTROL, /* 绝对坐标系控制，控制在空间内的绝对姿态 */
  } Mode;

  typedef enum {
    SET_MODE_RELAX,
    SET_MODE_STABLE,
    SET_MODE_CONTROL,
  } GimbalEvent;

  typedef struct {
    Component::Type::Vector2 eulr_;
  } Setpoint;

  typedef struct {
    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param yaw_param;
    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param pitch_param;
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;
  } Param;

  Dartgimbal(Param &param, float control_freq);

  void UpdateFeedback();
  void Control();
  void SetMode(Mode mode);

 private:
  uint64_t last_wakeup_ = 0;
  uint64_t now_ = 0;
  float dt_ = 0.0f;

  Param param_;
  Setpoint setpoint_;
  Mode mode_ = RELAX;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1> yaw_;
  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1>
      pitch_;

  System::Thread thread_;
  System::Semaphore ctrl_lock_;
  Component::CMD::GimbalCMD cmd_;
};
}  // namespace Module
