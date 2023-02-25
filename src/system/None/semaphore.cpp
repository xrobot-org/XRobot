#include <semaphore.hpp>

#include "bsp_delay.h"

using namespace System;

Semaphore::Semaphore(bool init_count) : count_(init_count), max_count_(1) {}

Semaphore::Semaphore(uint16_t max_count, uint16_t init_count)
    : count_(init_count), max_count_(max_count) {}

void Semaphore::Give() {
  if (count_ < max_count_) {
    count_++;
  }
}

bool Semaphore::Take(uint32_t timeout) {
  while (!count_) {
    if (!timeout) {
      return false;
    }
    bsp_delay(1);
    timeout--;
  }
  count_--;
  return true;
}

void Semaphore::GiveFromISR() { Give(); }

bool Semaphore::TakeFromISR() { return Take(0); }
