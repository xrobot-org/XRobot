#include "queue.hpp"

using namespace System;

Queue::Queue(uint32_t item_size, uint16_t length) {
  this->handle_ = xQueueCreate(length, item_size);
}

Queue::Queue(const Queue& queue) : handle_(queue.handle_) {}

bool Queue::Send(const void* data, uint32_t timeout) {
  return xQueueSend(this->handle_, data, timeout) == pdTRUE;
}

bool Queue::Receive(void* buffer, uint32_t timeout) {
  return xQueueReceive(this->handle_, buffer, timeout) == pdTRUE;
}

bool Queue::Overwrite(void* buffer) {
  return xQueueOverwrite(this->handle_, buffer) == pdTRUE;
}

bool Queue::SendFromISR(const void* data) {
  return xQueueSendFromISR(this->handle_, data, NULL) == pdTRUE;
}

bool Queue::ReceiveFromISR(void* buffer) {
  return xQueueReceiveFromISR(this->handle_, buffer, NULL) == pdTRUE;
}

bool Queue::OverwriteFromISR(void* buffer) {
  return xQueueOverwriteFromISR(this->handle_, buffer, NULL) == pdTRUE;
}

bool Queue::Reset() { return xQueueReset(this->handle_) == pdTRUE; }
