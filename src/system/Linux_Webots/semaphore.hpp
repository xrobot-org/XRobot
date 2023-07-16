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
  Semaphore(uint16_t init_count) { sem_init(&this->handle_, 0, init_count); }

  void Post() { sem_post(&this->handle_); }

  bool Wait(uint32_t timeout = UINT32_MAX) {
    if (!sem_trywait(&this->handle_)) {
      return true;
    }

    if (!timeout) {
      return false;
    }

    uint32_t start_time = bsp_time_get_ms();
    bool ans = false;

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint32_t add = 0;
    long raw_time = 1U * 1000U * 1000U + ts.tv_nsec;
    add = raw_time / (1000U * 1000U * 1000U);

    ts.tv_sec += add;
    ts.tv_nsec = raw_time % (1000U * 1000U * 1000U);

    while (bsp_time_get_ms() - start_time < timeout) {
      ans = !sem_timedwait(&handle_, &ts);
      if (ans) {
        return true;
      }
    }

    return false;
  }

 private:
  sem_t handle_;
};
}  // namespace System
