#include <vector>

#include "comp_actuator.hpp"
#include "comp_cmd.hpp"
#include "dev_mech.hpp"
#include "dev_rm_motor.hpp"
#include "module.hpp"

namespace Module {
class DartLauncher {
 public:
  typedef struct {
    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param rod_actr;
    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param reload_actr;
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;

    std::array<Component::SpeedActuator::Param, 4> fric_actr;
    std::array<Device::RMMotor::Param, 4> fric_motor;
  } Param;

  typedef struct {
    float rod;
    float reload;
    float fric_speed;
  } Setpoint;

  typedef enum {
    RELOAD,
    RESET,
    ON,
    OFF,
  } Event;

  DartLauncher(Param& param, float control_freq);

  void UpdateFeedback();

  void Control();

 private:
  Param& param_;

  Setpoint setpoint_;

  float dt_ = 0.0f;

  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;

  uint32_t last_reload_time_ = 0;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1>
      rod_actr_;
  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1>
      reload_actr_;

  std::array<Component::SpeedActuator*, 4> fric_actr_;
  std::array<Device::RMMotor*, 4> fric_motor_;

  std::array<float, 4> motor_out_;

  System::Thread thread_;
};
}  // namespace Module
