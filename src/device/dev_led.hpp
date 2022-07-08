#pragma once

#include "dev.hpp"

namespace Device {
class LED {
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

  LED();

  bool Set(Channel ch, Status status, float duty_cycle);

  System::Thread thread_;
};
}  // namespace Device
