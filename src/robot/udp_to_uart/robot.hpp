#include "comp_utils.hpp"
#include "dev_blink_led.hpp"
#include "dev_net_config.hpp"
#include "mod_topic_share_uart.hpp"
#include "mod_uart_udp.hpp"

void robot_init();
namespace Robot {
class UdpToUart {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
    Module::UartToUDP::Param uart_udp;
    Module::TopicShareClientUart::Param topic_share;
  } Param;

  Device::BlinkLED led_;
  Module::UartToUDP uart_udp_;
  Device::NetConfig net_config_;
  Module::TopicShareClientUart topic_share_;

  UdpToUart(Param& param)
      : led_(param.led),
        uart_udp_(param.uart_udp),
        topic_share_(param.topic_share) {}
};
}  // namespace Robot
