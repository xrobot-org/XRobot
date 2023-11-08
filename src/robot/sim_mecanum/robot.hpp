#include "dev_blink_led.hpp"
#include "dev_camera.hpp"
#include "dev_cap.hpp"
#include "dev_controller.hpp"
#include "dev_imu.hpp"
#include "mod_chassis.hpp"

void robot_init();

namespace Robot {
class Simulator {
 public:
  typedef struct Param {
    Device::BlinkLED::Param led{};
    Device::IMU::Param imu{};
    Device::Cap::Param cap{};
    Module::RMChassis::Param chassis{};
  } Param;

  Component::CMD cmd_;

  Device::BlinkLED led_;
  Device::Cap cap_;
  Device::Referee referee_;
  Device::TerminalController ctrl_;
  Device::Camera camera_;
  Device::IMU imu_;

  Module::RMChassis chassis_;

  Simulator(Param& param)
      : cmd_(Component::CMD::CMD_TERM_CTRL),
        led_(param.led),
        cap_(param.cap),
        imu_(param.imu),
        chassis_(param.chassis, 500) {}
};
}  // namespace Robot
