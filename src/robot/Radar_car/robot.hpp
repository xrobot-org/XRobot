#include "comp_cmd.hpp"
#include "dev_ai.hpp"
#include "dev_can.hpp"
#include "dev_dr16.hpp"
#include "dev_led_rgb.hpp"
#include "mod_Radar.hpp"

void robot_init();
namespace Robot {
class Radar_car {
public:
  typedef struct Param {
    Module::Radar_car::Param chassis{};
  } Param;

  Component::CMD cmd_;

  Device::AI ai_;

  Device::Can can_;
  Device::DR16 dr16_;
  Device::RGB led_;

  Module::Radar_car chassis_;

  Radar_car(Param& param, float control_freq)
      : cmd_(Component::CMD::CMD_AUTO_CTRL),
        ai_(false),
        chassis_(param.chassis, control_freq) {}
};
}  // namespace Robot
