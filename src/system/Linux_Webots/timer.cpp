#include <timer.hpp>

using namespace System;

Timer* Timer::self_ = NULL;

Timer::Timer() {
  self_ = this;

  auto thread_fn = [](void* arg) {
    (void)arg;
    while (1) {
      Timer::self_->list_.Foreach(Timer::Refresh, NULL);
      Timer::self_->thread_.SleepUntil(1);
    }
  };

  this->thread_.Create(thread_fn, static_cast<void*>(NULL), "timer_task", 256,
                       Thread::MEDIUM);
}

bool Timer::Refresh(ControlBlock& block, void* arg) {
  (void)arg;

  block.count++;
  if (block.cycle <= block.count) {
    block.count = 0;
    block.fun(block.type);
  }

  return true;
}
