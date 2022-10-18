#pragma once

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

namespace System {
template <typename Data>
class Queue {
 public:
  Queue(uint16_t length) { this->handle_ = xQueueCreate(length, sizeof(Data)); }

  bool Send(const Data& data, uint32_t timeout) {
    return xQueueSend(this->handle_, &data, timeout) == pdTRUE;
  }

  bool Receive(Data& data, uint32_t timeout) {
    return xQueueReceive(this->handle_, &data, timeout) == pdTRUE;
  }

  bool Overwrite(const Data& data) {
    return xQueueOverwrite(this->handle_, &data) == pdTRUE;
  }

  bool SendFromISR(const Data& data) {
    return xQueueSendFromISR(this->handle_, &data, NULL) == pdTRUE;
  }

  bool ReceiveFromISR(Data& data) {
    return xQueueReceiveFromISR(this->handle_, &data, NULL) == pdTRUE;
  }

  bool OverwriteFromISR(const Data& data) {
    return xQueueOverwriteFromISR(this->handle_, &data, NULL) == pdTRUE;
  }

  bool Reset() { return xQueueReset(this->handle_) == pdTRUE; }

 private:
  QueueHandle_t handle_;
};
}  // namespace System
