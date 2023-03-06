#include <comp_cmd.hpp>

#include "dev_blink_led.hpp"
#include "dev_can.hpp"
#include "dev_dr16.hpp"
#include "mod_ore_collect.hpp"

void robot_init();

namespace Robot {
class Engineer {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
    Module::OreCollect::Param ore_collect;
  } Param;

  Component::CMD cmd_;
  Device::BlinkLED led_;
  Device::Can can_;
  Device::DR16 dr16_;
  Module::OreCollect ore_collect_;

  static Engineer* self_;

  Engineer(Param& param, float control_freq)
      : led_(param.led), ore_collect_(param.ore_collect, control_freq) {
    self_ = this;
  }
};
}  // namespace Robot
