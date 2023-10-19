#pragma once

#include <cstdint>
#include <thread.hpp>

#include "bsp_def.h"
#include "bsp_sys.h"

static uint32_t signal_value;

namespace System {
class Signal {
 public:
  static bool Action(System::Thread& thread, int sig) {
    XB_UNUSED(thread);

    signal_value |= 1 << sig;

    return true;
  }

  static bool Wait(int sig, uint32_t timeout) {
    while (timeout--) {
      if (((1 << sig) | signal_value) == 1 << sig) {
        signal_value &= ~1 << sig;
        return true;
      }
      Thread::Sleep(1);
    }

    return false;
  }
};

}  // namespace System
