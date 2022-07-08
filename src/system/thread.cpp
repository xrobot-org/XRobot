#include "thread.hpp"

using namespace System;

void Thread::Create(Function fn, const char* name, uint32_t stack_depth,
                    Priority priority, void* arg) {
  xTaskCreate(fn, name, stack_depth, arg, priority, &(this->handle_));
}

void Thread::Sleep(uint32_t microseconds) { vTaskDelay(microseconds); }

uint32_t Thread::GetTick() { return xTaskGetTickCount(); }

void Thread::Stop() { vTaskSuspend(this->handle_); }

void Thread::StartKernel() {
#ifndef CONFIG_OS_FREERTOS
  vTaskStartScheduler();
#else
#endif
}
