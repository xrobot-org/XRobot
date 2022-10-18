#include "thread.hpp"

using namespace System;

void Thread::Stop() { vTaskSuspend(this->handle_); }

void Thread::StartKernel() {
#ifndef CONFIG_OS_FREERTOS
  vTaskStartScheduler();
#else
#endif
}
