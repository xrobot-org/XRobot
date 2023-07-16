#pragma once

#include <cstdint>
#include <thread.hpp>

#include "bsp_def.h"
#include "bsp_sys.h"

namespace System {
class Signal {
 public:
  static bool Action(System::Thread& thread, int sig) {
    XB_ASSERT(sig >= 0 && sig < 32);
    if (bsp_sys_in_isr()) {
      BaseType_t px_higher_priority_task_woken = 0;
      BaseType_t ans = xTaskNotifyFromISR(thread.handle_, 1 << sig, eSetBits,
                                          &px_higher_priority_task_woken);
      if (px_higher_priority_task_woken != pdFALSE) {
        portYIELD();
      }
      return ans == pdPASS;
    } else {
      return xTaskNotify(thread.handle_, 1 << sig, eSetBits) == pdPASS;
    }
  }

  static bool Wait(int sig, uint32_t timeout) {
    XB_ASSERT(sig >= 0 && sig < 32);

    uint32_t value = 0;
    xTaskNotifyAndQuery(xTaskGetCurrentTaskHandle(), 0, eNoAction, &value);

    const uint32_t SIG_BIT = 1 << sig;

    if ((value & SIG_BIT) == SIG_BIT) {
      value &= ~SIG_BIT;
      xTaskNotify(xTaskGetCurrentTaskHandle(), value, eSetValueWithOverwrite);
      return true;
    } else {
      if (timeout == 0) {
        return false;
      }
    }

    uint32_t current_time = xTaskGetTickCount();

    while (xTaskNotifyWait(0, SIG_BIT, &value, timeout) == pdPASS) {
      if ((value & SIG_BIT) == SIG_BIT) {
        return true;
      }

      uint32_t now = xTaskGetTickCount();

      if (now - current_time >= timeout) {
        return false;
      }

      timeout -= now - current_time;
    }

    return false;
  }
};

}  // namespace System
