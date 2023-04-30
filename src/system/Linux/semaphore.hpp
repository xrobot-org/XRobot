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
  Semaphore(bool init_count) : max_count_(1) {
    sem_init(&this->handle_, 0, init_count);
  }

  Semaphore(uint16_t max_count, uint16_t init_count) : max_count_(max_count) {
    sem_init(&this->handle_, 0, init_count);
  }

  void Give() {
    int tmp = 0;
    sem_getvalue(&this->handle_, &tmp);
    if (tmp < this->max_count_) {
      sem_post(&this->handle_);
    }
  }

  bool Take(uint32_t timeout) {
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

  void GiveFromISR() { Give(); }
  bool TakeFromISR() { return Take(0); }

 private:
  sem_t handle_;
  int max_count_;
};
}  // namespace System
