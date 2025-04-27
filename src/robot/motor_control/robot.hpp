
#include "dev_blink_led.hpp"
#include "mod_launcher.hpp"
#include "mod_speed_control.hpp"

void robot_init();

namespace Robot {
class MotorCtrl {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
    Module::SpeedControl::Param speed_ctrl;
  } Param;

  MotorCtrl(Param& param) : led_(param.led), speed_control_(param.speed_ctrl) {}

  Device::BlinkLED led_; /* led设备 */
  Device::Can can_;      /* CAN设备 */

  Module::SpeedControl speed_control_; /* 速度控制模块 */
};
}  // namespace Robot
