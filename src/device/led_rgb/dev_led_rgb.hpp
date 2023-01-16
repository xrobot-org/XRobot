#pragma once

#include <device.hpp>

namespace Device {
class RGB {
 public:
  typedef enum {
    ON,
    OFF,
    TAGGLE,
  } Status;

  typedef enum {
    BLUE,
    RED,
    GREEN,
    CHANNEL_NUMBER,
  } Channel;

  RGB();

  bool Set(Channel ch, Status status, float duty_cycle);

  System::Thread thread_;
};
}  // namespace Device
