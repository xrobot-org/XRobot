#pragma once

#include <device.hpp>

namespace Device {
class Laser {
 public:

  Laser(bool auto_start = true);

  void Start();

  bool Set(float duty_cycle);

  void Stop();

 private:
  System::Thread  thread_;

};
}  // namespace Device
