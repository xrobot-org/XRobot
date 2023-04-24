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
    if (!sem_trywait(&this->handle_)) {
      return true;
    }

    if (!timeout) {
      return false;
    }

    uint32_t start_time = bsp_time_get_ms();
    bool ans = false;

    while (bsp_time_get_ms() - start_time < timeout) {
      ans = !sem_trywait(&this->handle_);
      if (ans) {
        return true;
      }
      System::Thread::Sleep(1);
    }

    return false;
  }

  void GiveFromISR() { Give(); }
  bool TakeFromISR() { return Take(0); }

 private:
  sem_t handle_;
  int max_count_;
};
}  // namespace System
