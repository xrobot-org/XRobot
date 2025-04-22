#include "comp_cmd.hpp"
#include "dev_mech.hpp"
#include "module.hpp"

namespace Module {
class DartLauncher {
 public:
  typedef struct {
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;
    Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param,
                                1>::Param rod_;
    std::array<Component::SpeedActuator::Param, 4> fric_actr;
    std::array<Device::RMMotor::Param, 4> fric_motor;
  } Param;
  typedef enum {
    RELAX,
    OFF,
    ROD_ON,
    FRIC_ON,
    ON,
    STAY,
    ADVANCE,
  } Mode;
  typedef enum {
    SET_MODE_RELAX,
    SET_MODE_OFF,
    SET_MODE_ROD,
    SET_MODE_FRIC,
    SET_MODE_ON,
    SET_MODE_STAY,
    SET_MODE_ADVANCE,
  } Event;
  typedef struct {
    float rod_pos;
    float fric_speed1;
    float fric_speed2;
  } Setpoint;
  DartLauncher(Param& param, float control_freq);
  void Feedback();
  void Control();
  void SetMode(Mode mode);

 private:
  System::Semaphore ctrl_lock_;
  System::Thread thread_;
  Param& param_;
  Setpoint setpoint_;

  float dt_;
  uint64_t last_wakeup_;
  uint64_t now_;

  bool relax_ = true;
  int rod_pos_ = 0;
  Mode mode_ = RELAX;

  Device::AutoCaliLimitedMech<Device::RMMotor, Device::RMMotor::Param, 1> rod_;

  std::array<Component::SpeedActuator*, 4> fric_actr_;
  std::array<Device::RMMotor*, 4> fric_motor_;

  std::array<float, 4> fric_out_;
};
}  // namespace Module
