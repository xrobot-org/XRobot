#pragma once

#include "mod_config.h"

typedef struct {
  TaskFunction_t fn;
  const char* name;
  configSTACK_DEPTH_TYPE stack_depth;
  UBaseType_t priority;
} thd_t;

#define THREAD_DECLEAR(_fn, _stack_depth, _priority) \
  static const thd_t thd_init_##_fn                  \
      __attribute__((section(".thread"), used)) = {  \
          .fn = _fn,                                 \
          .name = #_fn,                              \
          .stack_depth = _stack_depth,               \
          .priority = _priority,                     \
  }

typedef struct {
  /* 机器人状态 */
  struct {
    float battery;
    float vbat;
    float cpu_temp;
  } status;

  config_t cfg; /* 机器人配置 */

#ifdef MCU_DEBUG_BUILD

#endif
} runtime_t;

void thd_init(void);
