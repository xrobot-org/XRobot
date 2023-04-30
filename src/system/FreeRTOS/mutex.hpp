#pragma once

#include <cstdint>
#include <cstdio>
#include <semaphore.hpp>
#include <thread.hpp>

#include "bsp_time.h"

namespace System {
class Mutex {
 public:
  Mutex(bool unlock = true) : sem_(1, unlock) {}

  void Unlock() { sem_.Give(); }

  bool Lock(uint32_t timeout) { return sem_.Take(timeout); }

  void UnlockFromISR() { sem_.Give(); }

  bool LockFromISR() { return sem_.Take(UINT32_MAX); }

 private:
  System::Semaphore sem_;
};
}  // namespace System
