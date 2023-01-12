#pragma once

#include <stdint.h>

#include <string>

#include "FreeRTOS.h"
#include "bsp_time.h"
#include "system.hpp"
#include "task.h"

namespace System {
class Thread {
 public:
  typedef enum { IDLE, Low, Medium, High, Realtime } Priority;

  template <typename FunType, typename ArgType>
  void Create(FunType fun, ArgType arg, const char* name, uint32_t stack_depth,
              Priority priority) {
    static_cast<void (*)(ArgType)>(fun);

    TypeErasure<void, ArgType>* type = static_cast<TypeErasure<void, ArgType>*>(
        pvPortMalloc(sizeof(TypeErasure<void, ArgType>)));

    *type = TypeErasure<void, ArgType>(fun, arg);

    xTaskCreate(type->Port, name, stack_depth, type, priority,
                &(this->handle_));
  }

  static void Sleep(uint32_t microseconds) { vTaskDelay(microseconds); }

  void SleepUntil(uint32_t microseconds) {
    xTaskDelayUntil(&last_weakup_tick_, microseconds);
  }

  void Stop();

  static void StartKernel();

 private:
  TaskHandle_t handle_ = NULL;
  uint32_t last_weakup_tick_ = 0;
};
}  // namespace System
