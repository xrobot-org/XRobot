#include <timer.hpp>

#include "bsp_time.h"

using namespace System;

Timer* Timer::self_ = NULL;

Timer::Timer() {
  self_ = this;

  auto thread_fn = [](void* arg) {
    XB_UNUSED(arg);
    uint32_t last_online_time = bsp_time_get_ms();
    while (1) {
      Timer::self_->list_.Foreach(Timer::Refresh, NULL);
      Timer::self_->thread_.SleepUntil(1, last_online_time);
    }
  };

  this->thread_.Create(thread_fn, static_cast<void*>(NULL), "timer_task",
                       FREERTOS_TIMER_TASK_STACK_DEPTH, Thread::HIGH);
}

bool Timer::Refresh(ControlBlock& block, void* arg) {
  XB_UNUSED(arg);

  if (!block.running) {
    return true;
  }

  block.count++;
  if (block.cycle <= block.count) {
    block.count = 0;
    block.fun(block.type);
  }

  return true;
}
