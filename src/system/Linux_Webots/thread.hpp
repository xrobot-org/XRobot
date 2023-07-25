#pragma once

#include <poll.h>
#include <pthread.h>
#include <stdint.h>

#include <csignal>
#include <cstdint>
#include <cstring>
#include <memory.hpp>
#include <string>

#include "bsp_def.h"
#include "bsp_time.h"
#include "system_ext.hpp"

namespace System {
class Thread {
 public:
  typedef enum { IDLE, LOW, MEDIUM, HIGH, REALTIME } Priority;

  Thread(){};
  Thread(pthread_t handle) : handle_(handle){};

  template <typename FunType, typename ArgType>
  void Create(FunType fun, ArgType arg, const char* name, size_t stack_depth,
              Priority priority) {
    XB_UNUSED(name);
    XB_UNUSED(priority);

    XB_UNUSED(static_cast<void (*)(ArgType)>(fun));

    class ThreadBlock {
     public:
      ThreadBlock(FunType fun, ArgType arg, const char* name)
          : type_(fun, arg),
            name_(reinterpret_cast<char*>(
                System::Memory::Malloc(strlen(name) + 1))) {
        strcpy(name_, name);
      }
      TypeErasure<void, ArgType> type_;
      char* name_;
    };

    auto block = new ThreadBlock(fun, arg, name);

    auto port = [](void* arg) {
      ThreadBlock* block = static_cast<ThreadBlock*>(arg);
      const char* name = block->name_;
      XB_UNUSED(name);
      sigset_t waitset;
      sigfillset(&waitset);
      pthread_sigmask(SIG_BLOCK, &waitset, NULL);
      block->type_.fun_(block->type_.arg_);
      return static_cast<void*>(NULL);
    };

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stack_depth > 256) {
      pthread_attr_setstacksize(&attr, stack_depth / 1024 * 4);
    } else {
      pthread_attr_setstacksize(&attr, 1);
    }
    pthread_create(&this->handle_, &attr, port, block);
  }

  static Thread Current(void) { return Thread(pthread_self()); }

  static void Sleep(uint32_t microseconds) {
    uint32_t time = bsp_time_get_ms() + microseconds;

    while (bsp_time_get_ms() < time) {
      poll(NULL, 0, 1);
    }
  }

  static void SleepMilliseconds(uint32_t microseconds) {
    uint32_t time = bsp_time_get_ms() + microseconds;

    while (bsp_time_get_ms() < time) {
      poll(NULL, 0, 1);
    }
  }

  static void SleepSeconds(uint32_t seconds) {
    float time = bsp_time_get() + static_cast<float>(seconds);

    while (bsp_time_get() < time) {
      poll(NULL, 0, 1);
    }
  }

  static void SleepMinutes(uint32_t minutes) { SleepSeconds(minutes * 60); }

  static void SleepHours(uint32_t hours) {
    while (hours--) {
      SleepMinutes(60);
    }
  }

  static void SleepDays(uint32_t days) {
    while (days--) {
      SleepHours(24);
    }
  }

  void SleepUntil(uint32_t microseconds, uint32_t& last_wakeup_time) {
    if (last_wakeup_time == 0) {
      last_wakeup_time = bsp_time_get_ms();
    }

    while (bsp_time_get_ms() - last_wakeup_time < microseconds) {
      poll(NULL, 0, 1);
    }
    last_wakeup_time += microseconds;
  }

  void Delete() { pthread_cancel(this->handle_); }

  static void Yield() { pthread_yield(); }

  pthread_t handle_;
};
}  // namespace System
