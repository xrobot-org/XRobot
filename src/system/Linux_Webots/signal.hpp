#pragma once

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <csignal>
#include <cstdint>
#include <thread.hpp>

#include "bsp_def.h"

namespace System {
class Signal {
 public:
  static bool Action(System::Thread& thread, int sig) {
    sig += SIGRTMIN;
    XB_ASSERT(sig >= SIGRTMIN && sig <= SIGRTMAX);
    return pthread_kill(thread.handle_, sig) == 0;
  }

  static bool Wait(int sig, uint32_t timeout) {
    sigset_t waitset, oldset;
    sig += SIGRTMIN;
    XB_ASSERT(sig >= SIGRTMIN && sig <= SIGRTMAX);

    uint32_t start_time = bsp_time_get_ms();

    sigemptyset(&waitset);
    sigaddset(&waitset, sig);
    pthread_sigmask(SIG_BLOCK, &waitset, &oldset);

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    uint32_t add = 0;
    long raw_time = 1U * 1000U * 1000U + ts.tv_nsec;
    add = raw_time / (1000U * 1000U * 1000U);

    ts.tv_sec += add;
    ts.tv_nsec = raw_time % (1000U * 1000U * 1000U);

    int res = sigtimedwait(&waitset, NULL, &ts);

    while (bsp_time_get_ms() - start_time < timeout) {
      res = !sigtimedwait(&waitset, NULL, &ts);
      if (res) {
        return true;
      }
    }
    pthread_sigmask(SIG_BLOCK, &oldset, NULL);
    return res == sig;
  }
};

}  // namespace System
