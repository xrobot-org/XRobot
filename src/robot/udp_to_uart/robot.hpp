#include "comp_utils.hpp"
#include "dev_blink_led.hpp"
#include "mod_uart_udp.hpp"

void robot_init();
namespace Robot {
class UdpToUart {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
    Module::UartToUDP::Param uart_udp;
  } Param;

  Device::BlinkLED led_;
  Module::UartToUDP uart_udp_;

  UdpToUart(Param& param) : led_(param.led), uart_udp_(param.uart_udp) {}
};
}  // namespace Robot
