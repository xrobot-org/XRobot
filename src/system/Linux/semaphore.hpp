#pragma once

#include <poll.h>
#include <semaphore.h>

#include <cstdint>
#include <cstdio>
#include <thread.hpp>

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
    while (sem_trywait(&this->handle_) && timeout) {
      System::Thread::Sleep(1);
      timeout--;
    }

    if (sem_trywait(&this->handle_)) {
      return false;
    } else {
      return true;
    }
  }

  void GiveFromISR() { Give(); }
  bool TakeFromISR() { return Take(0); }

 private:
  sem_t handle_;
  int max_count_;
};
}  // namespace System
