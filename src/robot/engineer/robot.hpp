#include <comp_cmd.hpp>

#include "dev_blink_led.hpp"
#include "dev_can.hpp"
#include "dev_can_imu.hpp"
#include "dev_dr16.hpp"
#include "dev_referee.hpp"
#include "mod_ore_collect.hpp"

void robot_init();

namespace Robot {
class Engineer {
 public:
  typedef struct Param {
    Device::BlinkLED::Param led{};
    Device::MicroSwitch::Param sw_2{};
    Device::MicroSwitch::Param sw_3{};
    Device::MicroSwitch::Param sw_4{};
    Device::IMU::Param imu{};
    Module::OreCollect::Param ore_collect{};
  } Param;

  Component::CMD cmd_;
  Device::BlinkLED led_;
  Device::Can can_{};
  Device::IMU imu_;
  Device::DR16 dr16_{};
  Device::Referee referee_{};
  Device::MicroSwitch sw_2;
  Device::MicroSwitch sw_3;
  Device::MicroSwitch sw_4;
  Module::OreCollect ore_collect_;

  static Engineer* self_;

  Engineer(Param& param, float control_freq)
      : led_(param.led),
        imu_(param.imu),
        sw_2(param.sw_2),
        sw_3(param.sw_3),
        sw_4(param.sw_4),
        ore_collect_(param.ore_collect, control_freq) {
    self_ = this;
  }
};
}  // namespace Robot
