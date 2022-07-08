#pragma once

#include <stdint.h>

#include <string>

#include "FreeRTOS.h"
#include "timers.h"

namespace System {
class Timer {
 public:
  typedef void (*Function)(TimerHandle_t arg);

  Timer(std::string name, uint32_t millisec, Function fn);

  bool Start();

 private:
  TimerHandle_t handle_;
  uint32_t ticks_;
};
}  // namespace System
