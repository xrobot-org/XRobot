#include "comp_utils.hpp"
#include "dev_blink_led.hpp"
#include "dev_controller.hpp"
#include "mod_chassis.hpp"
#include "om.hpp"
#include "term.hpp"

void robot_init();

namespace Robot {
class Simulator {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
    Module::RMChassis::Param chassis;
  } Param;

  Message message_;

  System::Term term_;

  Component::CMD cmd_;

  Device::BlinkLED led_;
  Device::Referee referee_;
  Device::TerminalController ctrl_;

  Module::RMChassis chassis_;

  Simulator(Param& param)
      : cmd_(Component::CMD::TerminalControl),
        led_(param.led),
        chassis_(param.chassis, 500) {}
};
}  // namespace Robot
