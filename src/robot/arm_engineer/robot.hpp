#include "comp_cmd.hpp"
#include "dev_blink_led.hpp"
#include "dev_can.hpp"
#include "dev_damiaomotor.hpp"
#include "dev_dr16.hpp"
#include "dev_motor.hpp"
#include "dev_referee.hpp"
#include "mod_engineer_chassis.hpp"
#include "mod_robot_arm.hpp"
void robot_init();

namespace Robot {
class ArmEngineer {
 public:
  typedef struct {
    Device::BlinkLED::Param blink;
    Module::RMChassis::Param chassis;
    Module::RobotArm::Param robotarm;

  } Param;

  Component::CMD cmd_;

  Device::Can can_;
  Device::Referee referee_;
  Device::DR16 dr16_;
  Device::BlinkLED blink_;
  Module::RMChassis chassis_;
  Module::RobotArm robotarm_;

  ArmEngineer(Param &param, float control_freq)
      : cmd_(Component::CMD::CMD_OP_CTRL),
        blink_(param.blink),
        chassis_(param.chassis, control_freq),
        robotarm_(param.robotarm, control_freq) {}
};
}  // namespace Robot
