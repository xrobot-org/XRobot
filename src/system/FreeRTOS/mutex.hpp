#pragma once

#include <cstdint>
#include <cstdio>
#include <thread.hpp>

#include "bsp_time.h"
#include "om.hpp"

namespace System {
class Mutex {
 public:
  Mutex(bool unlock = true) {
    om_mutex_init(&this->handle_);
    if (!unlock) {
      om_mutex_lock(&this->handle_);
    }
  }

  void Unlock() { om_mutex_unlock(&this->handle_); }

  bool Lock(uint32_t timeout) {
    if ((om_mutex_trylock(&this->handle_)) == OM_OK) {
      return true;
    }

    if (!timeout) {
      return false;
    }

    uint32_t start_time = bsp_time_get_ms();
    bool ans = false;

    while (bsp_time_get_ms() - start_time < timeout) {
      ans = (om_mutex_trylock(&this->handle_)) == OM_OK;
      if (ans) {
        return true;
      }
      System::Thread::Sleep(1);
    }

    return false;
  }

  void UnlockFromISR() {
    BaseType_t px_higher_priority_task_woken = 0;
    xSemaphoreGiveFromISR(this->handle_, &px_higher_priority_task_woken);
    if (px_higher_priority_task_woken != pdFALSE) {
      portYIELD();
    }
  }

  bool LockFromISR() {
    BaseType_t px_higher_priority_task_woken = 0;
    bool ans = xSemaphoreTakeFromISR(this->handle_,
                                     &px_higher_priority_task_woken) == pdTRUE;

    if (px_higher_priority_task_woken != pdFALSE) {
      portYIELD();
    }
    return ans;
  }

 private:
  om_mutex_t handle_;
};
}  // namespace System
