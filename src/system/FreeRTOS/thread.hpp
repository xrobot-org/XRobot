#pragma once

#include <cstdint>
#include <string>

#include "FreeRTOS.h"
#include "bsp_time.h"
#include "system_ext.hpp"
#include "task.h"

namespace System {
class Thread {
 public:
  typedef enum { IDLE, LOW, MEDIUM, HIGH, REALTIME } Priority;

  Thread(){};
  Thread(TaskHandle_t handle) : handle_(handle){};

  template <typename FunType, typename ArgType>
  void Create(FunType fun, ArgType arg, const char* name, uint32_t stack_depth,
              Priority priority) {
    (void)static_cast<void (*)(ArgType)>(fun);

    TypeErasure<void, ArgType>* type = static_cast<TypeErasure<void, ArgType>*>(
        pvPortMalloc(sizeof(TypeErasure<void, ArgType>)));

    *type = TypeErasure<void, ArgType>(fun, arg);

    xTaskCreate(type->Port, name, stack_depth, type, priority,
                &(this->handle_));
  }

  static Thread Current(void) { return Thread(xTaskGetCurrentTaskHandle()); }

  static void Sleep(uint32_t microseconds) { vTaskDelay(microseconds); }

  static void SleepMilliseconds(uint32_t microseconds) {
    vTaskDelay(microseconds);
  }

  static void SleepSeconds(uint32_t seconds) { vTaskDelay(seconds * 1000); }

  static void SleepMinutes(uint32_t minutes) { SleepSeconds(minutes * 60); }

  static void SleepHours(uint32_t hours) {
    while (hours--) {
      SleepMinutes(60);
    }
  }

  static void SleepDays(uint32_t days) {
    while (days--) {
      SleepHours(24);
    }
  }

  void SleepUntil(uint32_t microseconds, uint32_t& last_wakeup_time) {
    vTaskDelayUntil(&last_wakeup_time, microseconds);
  }

  void Delete() { vTaskDelete(this->handle_); }

  static void Yield() { taskYIELD(); }

  TaskHandle_t handle_ = NULL;
};
}  // namespace System
