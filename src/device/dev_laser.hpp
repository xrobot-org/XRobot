#pragma once

#include "dev.hpp"

namespace Device {
class Laser {
  void Start();

  bool Set(float duty_cycle);

  void Stop();
};
}  // namespace Device
