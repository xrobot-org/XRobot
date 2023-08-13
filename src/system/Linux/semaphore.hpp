#pragma once

#include <poll.h>
#include <semaphore.h>

#include <cstdint>
#include <cstdio>
#include <thread.hpp>

#include "bsp_time.h"

namespace System {
class Semaphore {
 public:
  Semaphore(uint32_t init_count) { sem_init(&this->handle_, 0, init_count); }

  ~Semaphore() { sem_destroy(&this->handle_); }

  void Post() { sem_post(&this->handle_); }

  bool Wait(uint32_t timeout = UINT32_MAX) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint32_t secs = timeout / 1000;
    timeout = timeout % 1000;

    uint32_t add = 0;
    long raw_time = timeout * 1000U * 1000U + ts.tv_nsec;
    add = raw_time / (1000U * 1000U * 1000U);
    ts.tv_sec += (add + secs);
    ts.tv_nsec = raw_time % (1000U * 1000U * 1000U);

    return sem_timedwait(&handle_, &ts) == 0;
  }

  uint32_t Value() {
    int value = 0;
    sem_getvalue(&handle_, &value);
    return value;
  }

 private:
  sem_t handle_;
};
}  // namespace System
