#pragma once

#include <device.hpp>

#include "bsp_gpio.h"

namespace Device {
class BlinkLED {
 public:
  typedef struct {
    bsp_gpio_t gpio;
    uint32_t timeout;
  } Param;

  BlinkLED(Param& param);

  Param param_;

  bool state_;
};
}  // namespace Device
