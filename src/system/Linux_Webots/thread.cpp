#include <thread.hpp>

#include "bsp_time.h"
#include "webots/robot.h"
#include "webots/supervisor.h"

using namespace System;

void Thread::Sleep(uint32_t microseconds) {
  float time = bsp_time_get() + static_cast<float>(microseconds) / 1000.0f;

  while (bsp_time_get() < time) {
    poll(NULL, 0, 1);
  }
}

void Thread::SleepUntil(uint32_t microseconds) {
  float time = bsp_time_get() + static_cast<float>(microseconds) / 1000.0f;

  while (bsp_time_get() < time) {
    poll(NULL, 0, 1);
  }
}
