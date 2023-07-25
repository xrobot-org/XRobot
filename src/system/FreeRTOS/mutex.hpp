#pragma once

#include <semaphore.hpp>
#include <thread.hpp>

#include "bsp_time.h"

namespace System {
class Mutex {
 public:
  Mutex() : mutex_(xSemaphoreCreateMutex()) {}

  ~Mutex() { vSemaphoreDelete(mutex_); }

  void Unlock() { xSemaphoreGive(mutex_); }

  bool Lock() { return xSemaphoreTake(mutex_, UINT32_MAX); }

 private:
  SemaphoreHandle_t mutex_;
};
}  // namespace System
