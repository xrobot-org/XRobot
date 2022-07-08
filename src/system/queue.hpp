#pragma once

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

namespace System {
class Queue {
 public:
  Queue(const Queue& queue);
  Queue(uint32_t item_size, uint16_t length);
  bool Send(const void* data, uint32_t timeout);
  bool Receive(void* buffer, uint32_t timeout);
  bool Overwrite(void* buffer);
  bool SendFromISR(const void* data);
  bool ReceiveFromISR(void* buffer);
  bool OverwriteFromISR(void* buffer);
  bool Reset();

 private:
  QueueHandle_t handle_;
};
}  // namespace System
