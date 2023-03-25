#include "dev_blink_led.hpp"
#include "dev_can.hpp"
#include "mod_microswitch.hpp"

void robot_init();

namespace Robot {
class MicroSwitch {
 public:
  typedef struct {
    Device::BlinkLED::Param led;

  } Param;
  Device::BlinkLED led_;

  Device::Can can_;
  Module::MicroSwitch microswitch_;
  MicroSwitch(Param& param) : led_(param.led) {}
};
}  // namespace Robot
