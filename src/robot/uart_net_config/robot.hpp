#include "dev_blink_led.hpp"
#include "dev_net_config.hpp"
#include "mod_topic_share_uart.hpp"

void robot_init();
namespace Robot {
class NetConfig {
 public:
  typedef struct Param {
    Device::BlinkLED::Param led{};
    Module::TopicShareClientUart::Param topic_share;
  } Param;

  Device::BlinkLED led_;
  Device::NetConfig net_config_;
  Module::TopicShareClientUart topic_share_;

  NetConfig(Param& param) : led_(param.led), topic_share_(param.topic_share) {}
};
}  // namespace Robot
