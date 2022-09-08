#pragma once

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

namespace System {
class Semaphore {
 public:
  Semaphore(bool init_count);
  Semaphore(uint16_t max_count, uint16_t init_count);
  void Give();
  bool Take(uint32_t timeout);
  void GiveFromISR();
  bool TakeFromISR();

 private:
  SemaphoreHandle_t handle_;
};
}  // namespace System
