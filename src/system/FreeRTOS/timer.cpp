#include "timer.hpp"

using namespace System;

Timer::Timer(std::string name, uint32_t millisec, Function fn) {
  this->ticks_ = pdMS_TO_TICKS(millisec);
  this->handle_ =
      xTimerCreate(name.c_str(), pdMS_TO_TICKS(millisec), pdTRUE, NULL, fn);
}

bool Timer::Start() {
  return xTimerStart(this->handle_, this->ticks_) == pdTRUE;
}
