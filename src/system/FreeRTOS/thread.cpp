#include "thread.hpp"

using namespace System;

void Thread::Stop() { vTaskSuspend(this->handle_); }

void Thread::StartKernel() { vTaskStartScheduler(); }
