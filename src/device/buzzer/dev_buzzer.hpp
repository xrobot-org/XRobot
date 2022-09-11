#pragma once

namespace Device {
class Buzzer {
 public:
  bool Start(void);
  bool Stop(void);
  bool Set(float freq, float duty_cycle);
};
}  // namespace Device
