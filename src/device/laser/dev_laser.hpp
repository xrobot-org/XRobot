#pragma once

#include <device.hpp>

namespace Device {
class Laser {
  void Start();

  bool Set(float duty_cycle);

  void Stop();
};
}  // namespace Device
