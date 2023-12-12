#include "dev_blink_led.hpp"
#include "dev_can.hpp"
#include "dev_net_config.hpp"
#include "mod_topic_share_uart.hpp"
#include "mod_uart_udp.hpp"

void robot_init();
namespace Robot {
class UdpToUart {
 public:
  typedef struct Param {
    Device::BlinkLED::Param led{};
    Module::TopicShareClientUart::Param topic_share;
  } Param;

  Device::BlinkLED led_;
  Device::Can can_;
  Device::NetConfig net_config_;
  Module::TopicShareClientUart topic_share_;
  Module::UartToUDP uart_udp_;

  UdpToUart(Param& param) : led_(param.led), topic_share_(param.topic_share) {}
};
}  // namespace Robot
