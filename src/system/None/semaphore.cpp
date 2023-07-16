#include <semaphore.hpp>

#include "bsp_delay.h"
#include "stm32f1xx_hal.h"

using namespace System;

Semaphore::Semaphore(bool init_count) : count_(init_count), max_count_(1) {}

Semaphore::Semaphore(uint16_t max_count, uint16_t init_count)
    : count_(init_count), max_count_(max_count) {}

void Semaphore::Post() {
  if (count_ < max_count_) {
    count_++;
  }
}

bool Semaphore::Wait(uint32_t timeout) {
  uint32_t time = HAL_GetTick();
  while (!count_) {
    if (HAL_GetTick() - time >= timeout) {
      return false;
    }
  }
  count_--;
  return true;
}
