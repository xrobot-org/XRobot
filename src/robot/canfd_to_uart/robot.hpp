/* #include "dev_xxx.hpp" */

#include "dev_blink_led.hpp"
#include "dev_canfd.hpp"
void robot_init();

namespace Robot {
class CanFdToUart {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
  } Param;

  Device::BlinkLED led_;
  Device::Can can_;

  CanFdToUart(Param& param) : led_(param.led) {}
};
}  // namespace Robot
