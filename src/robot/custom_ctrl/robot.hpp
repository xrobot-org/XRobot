#include "dev_blink_led.hpp"
#include "dev_mt6701.hpp"
#include "mod_custom_controller.hpp"

void robot_init();
namespace Robot {
class CustomController {
 public:
  typedef struct Param {
    Device::BlinkLED::Param led{};
    Module::CustomController::Param custom_ctrl;
  } Param;

  Device::BlinkLED led_;

  Module::CustomController custom_ctrl;

  CustomController(Param& param)
      : led_(param.led),

        custom_ctrl(param.custom_ctrl) {}
};
}  // namespace Robot
