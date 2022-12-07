#include "dev_blink_led.hpp"

using namespace Device;

BlinkLED::BlinkLED(BlinkLED::Param& param) : param_(param) {
  auto led_thread = [](BlinkLED* led) {
    while (1) {
      bsp_gpio_write_pin(led->param_.gpio, true);

      led->thread_.SleepUntil(led->param_.timeout);

      bsp_gpio_write_pin(led->param_.gpio, false);

      led->thread_.SleepUntil(led->param_.timeout);
    }
  };

  this->thread_.Create(led_thread, this, "led_thread", 128,
                       System::Thread::Low);
}
