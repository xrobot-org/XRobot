#include <timer.hpp>

using namespace System;

Timer* Timer::self_ = NULL;

Timer::Timer() { self_ = this; }

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
