#include "comp_utils.hpp"
#include "dev_blink_led.hpp"

void robot_init();
namespace Robot {
class Blink {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
  } Param;

  Device::BlinkLED led_;

  Blink(Param& param) : led_(param.led) {}
};
}  // namespace Robot
