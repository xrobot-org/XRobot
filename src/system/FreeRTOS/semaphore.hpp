#pragma once

#include <cstdint>

#include "FreeRTOS.h"
#include "bsp_sys.h"
#include "semphr.h"

namespace System {
class Semaphore {
 public:
  Semaphore(uint32_t init_count)
      : handle_(xSemaphoreCreateCounting(UINT32_MAX, init_count)) {}

  ~Semaphore() { vSemaphoreDelete(handle_); }

  void Post() {
    if (bsp_sys_in_isr()) {
      BaseType_t px_higher_priority_task_woken = 0;
      xSemaphoreGiveFromISR(this->handle_, &px_higher_priority_task_woken);
      if (px_higher_priority_task_woken != pdFALSE) {
        portYIELD();
      }
    } else {
      xSemaphoreGive(this->handle_);
    }
  }

  bool Wait(uint32_t timeout = UINT32_MAX) {
    if (!bsp_sys_in_isr()) {
      return xSemaphoreTake(this->handle_, timeout) == pdTRUE;
    } else {
      BaseType_t px_higher_priority_task_woken = 0;
      BaseType_t ans =
          xSemaphoreTakeFromISR(this->handle_,
                                &px_higher_priority_task_woken) == pdTRUE;

      if (px_higher_priority_task_woken != pdFALSE) {
        portYIELD();
      }
      return ans == pdPASS;
    }
  }

  uint32_t Value() { return uxSemaphoreGetCount(&handle_); }

 private:
  SemaphoreHandle_t handle_;
};
}  // namespace System
