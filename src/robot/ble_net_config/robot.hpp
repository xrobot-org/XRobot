#include "comp_utils.hpp"
#include "dev_blink_led.hpp"
#include "mod_ble_net_config.hpp"
#include "mod_topic_share_uart.hpp"

void robot_init();
namespace Robot {
class NetConfig {
 public:
  typedef struct {
    Device::BlinkLED::Param led;
    Module::TopicShareServerUart<Module::BleNetConfig::Data>::Param topic_share;
  } Param;

  Device::BlinkLED led_;
  Module::BleNetConfig ble_net_config_;
  Module::TopicShareServerUart<Module::BleNetConfig::Data> topic_share_;

  NetConfig(Param& param) : led_(param.led), topic_share_(param.topic_share) {}
};
}  // namespace Robot
