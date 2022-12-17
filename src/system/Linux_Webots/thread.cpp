#include "thread.hpp"

#include "webots/robot.h"

using namespace System;

static bool thread_continue;

void Thread::StartKernel() {
  while (1) {
    wb_robot_step(1);
    thread_continue = true;
    poll(NULL, 0, 1);
    thread_continue = true;
  }
}

void Thread::Sleep(uint32_t microseconds) {
  while (microseconds) {
    poll(NULL, 0, 1);
    if (thread_continue) microseconds--;
  }
}

void Thread::SleepUntil(uint32_t microseconds) {
  while (microseconds) {
    poll(NULL, 0, 1);
    if (thread_continue) microseconds--;
  }
}
