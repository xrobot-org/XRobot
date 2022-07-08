#pragma once

#include <stdint.h>

#include <string>

#include "FreeRTOS.h"
#include "task.h"

#define THREAD_DECLEAR(_handle, _fn, _stack_depth, _priority, _arg) \
  do {                                                              \
    _handle.Create(_fn, #_fn, _stack_depth, _priority, _arg);       \
  } while (0)

namespace System {
class Thread {
 public:
  typedef enum { IDLE, Low, Medium, High, Realtime } Priority;

  typedef void (*Function)(void* arg);

  void Create(Function fn, const char* name, uint32_t stack_depth,
              Priority priority, void* arg);

  static void Sleep(uint32_t microseconds);

  void Stop();

  static uint32_t GetTick();

  static void StartKernel();

 private:
  TaskHandle_t handle_ = NULL;
};
}  // namespace System
