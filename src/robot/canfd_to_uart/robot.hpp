/* #include "dev_xxx.hpp" */

#include "dev_blink_led.hpp"
#include "dev_can.hpp"
#include "mod_canfd_to_uart.hpp"

void robot_init();

namespace Robot {
class CanFdToUart {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
  } Param;

  Device::BlinkLED led_;
  Device::Can can_;
  Module::FDCanToUart fdcan_to_uart;

  CanFdToUart(Param& param) : led_(param.led) {}
};
}  // namespace Robot
