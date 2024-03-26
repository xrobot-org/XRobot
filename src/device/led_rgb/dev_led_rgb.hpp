#pragma once

#include <device.hpp>

namespace Device {
class RGB {
 public:
  typedef enum {
    BLUE,
    RED,
    GREEN,
    CHANNEL_NUMBER,
  } Channel;

  RGB(bool auto_start = true);

  bool Set(Channel ch, float duty_cycle);

 private:
  System::Thread thread_;
};
}  // namespace Device
