#pragma once

#include "bsp_gpio.h"
#include "dev.hpp"

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
