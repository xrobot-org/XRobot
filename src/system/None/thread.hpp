#pragma once

#include <cstdint>
#include <string>

#include "bsp_delay.h"
#include "bsp_time.h"
#include "system_ext.hpp"

namespace System {
class Thread {
 public:
  typedef enum { IDLE, LOW, MEDIUM, HIGH, REALTIME } Priority;

  template <typename FunType, typename ArgType>
  void Create(FunType fun, ArgType arg, const char* name, uint32_t stack_depth,
              Priority priority) {
    (void)(fun, arg, name, stack_depth, priority);
  }

  static void Sleep(uint32_t microseconds) { bsp_delay(microseconds); }

  void SleepUntil(uint32_t microseconds) { bsp_delay(microseconds); }

  void Stop();
};
}  // namespace System
