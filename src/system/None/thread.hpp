#pragma once

#include <cstdint>
#include <string>

#include "bsp_def.h"
#include "bsp_time.h"
#include "system_ext.hpp"

namespace System {
class Thread {
 public:
  typedef enum { IDLE, LOW, MEDIUM, HIGH, REALTIME } Priority;

  template <typename FunType, typename ArgType>
  void Create(FunType fun, ArgType arg, const char* name, uint32_t stack_depth,
              Priority priority) {
    XB_UNUSED(fun);
    XB_UNUSED(arg);
    XB_UNUSED(name);
    XB_UNUSED(stack_depth);
    XB_UNUSED(priority);
  }

  static Thread Current(void) { return Thread(); }

  static void Sleep(uint32_t microseconds) {
    auto last_time = bsp_time_get_ms();
    while ((bsp_time_get_ms() - last_time) < microseconds) {
    }
  }

  static void SleepMilliseconds(uint32_t microseconds) { Sleep(microseconds); }

  static void SleepSeconds(uint32_t seconds) {
    SleepMilliseconds(seconds * 1000);
  }

  static void SleepMinutes(uint32_t minutes) { SleepSeconds(minutes * 60); }

  static void SleepHours(uint32_t hours) {
    while (hours--) {
      SleepMinutes(60);
    }
  }

  static void SleepDays(uint32_t days) {
    while (days--) {
      SleepHours(24);
    }
  }

  void SleepUntil(uint32_t microseconds, uint32_t& last_time) {
    while ((bsp_time_get_ms() - last_time) < microseconds) {
    }

    last_time += microseconds;
  }

  void Delete() {}

  static void Yield() {}
};
}  // namespace System
