#include "comp_utils.hpp"
#include "dev_blink_led.hpp"
#include "dev_camera.hpp"
#include "dev_controller.hpp"
#include "dev_imu.hpp"
#include "mod_balance.hpp"

void robot_init();

namespace Robot {
class Simulator {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
    Device::IMU::Param imu;
    Module::RMBalance::Param chassis;
  } Param;

  Component::CMD cmd_;

  Device::BlinkLED led_;
  Device::Referee referee_;
  Device::TerminalController ctrl_;
  Device::Camera camera_;
  Device::IMU imu_;

  Module::RMBalance chassis_;

  Simulator(Param& param)
      : cmd_(Component::CMD::CMD_TERM_CTRL),
        led_(param.led),
        imu_(param.imu),
        chassis_(param.chassis, 500) {}
};
}  // namespace Robot
