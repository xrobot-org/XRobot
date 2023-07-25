#pragma once

#include <pthread.h>

#include <cstdint>
#include <cstdio>
#include <semaphore.hpp>
#include <thread.hpp>

#include "bsp_time.h"

namespace System {
class Mutex {
 public:
  Mutex() {}

  ~Mutex() {}

  void Unlock() { pthread_mutex_unlock(&mutex_); }

  bool Lock() { return pthread_mutex_lock(&mutex_) == 0; }

 private:
  pthread_mutex_t mutex_ = PTHREAD_MUTEX_INITIALIZER;
};
}  // namespace System
