#include "dev_blink_led.hpp"

using namespace Device;

BlinkLED::BlinkLED(BlinkLED::Param& param) : param_(param), state_(false) {
  auto led_thread = [](BlinkLED* led) {
    bsp_gpio_write_pin(led->param_.gpio, led->state_);
    led->state_ = !led->state_;
  };

  System::Timer::Create(led_thread, this, param.timeout);
}
