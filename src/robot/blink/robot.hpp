#include "dev_blink_led.hpp"
#include "mod_performance.hpp"

void robot_init();
namespace Robot {
class Blink {
 public:
  typedef struct Param {
    Device::BlinkLED::Param led{};
  } Param;

  Device::BlinkLED led_;

  Module::Performance perf_;

  Blink(Param& param) : led_(param.led) {}
};
}  // namespace Robot
