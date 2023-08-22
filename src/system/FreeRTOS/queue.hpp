#pragma once

#include <cstdint>
#include <mutex.hpp>
#include <semaphore.hpp>
#include <thread.hpp>

#include "FreeRTOS.h"
#include "bsp_sys.h"
#include "bsp_time.h"
#include "om.hpp"
#include "projdefs.h"
#include "task.h"

namespace System {
template <typename Data>
class Queue {
 public:
  Queue(uint16_t length) : queue_(xQueueCreate(length, sizeof(Data))) {}

  bool Send(const Data& data) {
    if (bsp_sys_in_isr()) {
      BaseType_t xHigherPriorityTaskWoken;
      auto ans = xQueueSendFromISR(queue_, &data, &xHigherPriorityTaskWoken);
      if (xHigherPriorityTaskWoken) {
        /* Actual macro used here is port specific. */
        portYIELD();
      }
      return ans == pdTRUE;
    } else {
      return xQueueSend(queue_, &data, 0) == pdTRUE;
    }
  }

  bool Receive(Data& data) {
    if (bsp_sys_in_isr()) {
      BaseType_t xHigherPriorityTaskWoken;
      auto ans = xQueueReceiveFromISR(queue_, &data, &xHigherPriorityTaskWoken);
      if (xHigherPriorityTaskWoken) {
        portYIELD();
      }
      return ans == pdTRUE;
    } else {
      return xQueueReceive(queue_, &data, 0) == pdTRUE;
    }
  }

  bool Overwrite(const Data& data) {
    if (bsp_sys_in_isr()) {
      BaseType_t xHigherPriorityTaskWoken;

      auto ans =
          xQueueOverwriteFromISR(queue_, &data, &xHigherPriorityTaskWoken);
      if (xHigherPriorityTaskWoken) {
        portYIELD();
      }
      return ans == pdTRUE;
    } else {
      return xQueueOverwrite(queue_, &data) == pdTRUE;
    }
  }

  bool Reset() { return xQueueReset(queue_) == pdTRUE; }

  uint32_t Size() {
    if (bsp_sys_in_isr()) {
      return uxQueueMessagesWaitingFromISR(queue_);
    } else {
      return uxQueueMessagesWaiting(queue_);
    }
  }

 private:
  QueueHandle_t queue_;
};
}  // namespace System
