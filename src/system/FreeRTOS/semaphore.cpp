#include <semaphore.hpp>

using namespace System;

Semaphore::Semaphore(bool init_count) : handle_(xSemaphoreCreateBinary()) {
  if (init_count) {
    this->Give();
  }
}

Semaphore::Semaphore(uint16_t max_count, uint16_t init_count)
    : handle_(xSemaphoreCreateCounting(max_count, init_count)) {}

void Semaphore::Give() { xSemaphoreGive(this->handle_); }

bool Semaphore::Take(uint32_t timeout) {
  return xSemaphoreTake(this->handle_, timeout) == pdTRUE;
}

void Semaphore::GiveFromISR() { xSemaphoreGiveFromISR(this->handle_, NULL); }

bool Semaphore::TakeFromISR() {
  return xSemaphoreTakeFromISR(this->handle_, NULL) == pdTRUE;
}
