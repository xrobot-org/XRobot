#include <vector>

#include "comp_cmd.hpp"
#include "dev_mech.hpp"
#include "dev_rm_motor.hpp"
#include "module.hpp"

namespace Module {
class OreCollect {
 public:
  typedef struct {
    const std::vector<Component::CMD::EventMapItem> EVENT_MAP;
    Device::LinearMech<Device::RMMotor, Device::MicroSwitchLimit, 1>::Param
        x_actr;
    Device::SteeringMech<Device::RMMotor, Device::MicroSwitchLimit, 1>::Param
        pitch_actr;
    Device::SteeringMech<Device::RMMotor, Device::MicroSwitchLimit, 1>::Param
        pitch_1_actr;
    Device::SteeringMech<Device::RMMotor, Device::MicroSwitchLimit, 1>::Param
        yaw_actr;
    Device::SteeringMech<Device::RMMotor, Device::MicroSwitchLimit, 1>::Param
        roll_actr;

    Device::LinearMech<Device::RMMotor, Device::MicroSwitchLimit, 2>::Param
        y_actr;
    Device::LinearMech<Device::RMMotor, Device::AutoReturnLimit, 2>::Param
        z_actr;
    Device::LinearMech<Device::RMMotor, Device::AutoReturnLimit, 2>::Param
        z_1_actr;

    Component::Type::Vector3 zero_position;
  } Param;

  typedef struct {
    float x;
    float pitch;
    float pitch_1;
    float yaw;
    float roll;
    float z;
    float z_1;
    float y;
  } Setpoint;

  typedef enum {
    RESET,
    FOLD,
    WORK,
    START_VACUUM,
    STOP_VACUUM,
    START_AUTO_COLLECT,
    STOP_AUTO_COLLECT,
  } Collect_Event;

  typedef enum { RELAX, CALI, MOVE } Mode;

  OreCollect(Param& param, float control_freq);

  void Control();

  void UpdateFeedback();

 private:
  System::Semaphore ctrl_lock_;

  Param& param_;

  float dt_ = 0.0f;

  uint64_t last_wakeup_ = 0;

  uint64_t now_ = 0;

  Mode mode_ = RELAX;

  Setpoint setpoint_;

  Component::Type::Eulr eulr_;

  Device::LinearMech<Device::RMMotor, Device::MicroSwitchLimit, 1> x_actr_;

  Device::SteeringMech<Device::RMMotor, Device::MicroSwitchLimit, 1>
      pitch_actr_;

  Device::SteeringMech<Device::RMMotor, Device::MicroSwitchLimit, 1>
      pitch_1_actr_;

  Device::SteeringMech<Device::RMMotor, Device::MicroSwitchLimit, 1> yaw_actr_;

  Device::SteeringMech<Device::RMMotor, Device::MicroSwitchLimit, 1> roll_actr_;

  Device::LinearMech<Device::RMMotor, Device::MicroSwitchLimit, 2> y_actr_;

  Device::LinearMech<Device::RMMotor, Device::AutoReturnLimit, 2> z_actr_;

  Device::LinearMech<Device::RMMotor, Device::AutoReturnLimit, 2> z_1_actr_;

  System::Thread thread_;
};
}  // namespace Module
