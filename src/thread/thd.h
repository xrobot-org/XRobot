/**
 * @file user_task.h
 * @author Qu Shen (503578404@qq.com)
 * @brief
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#include "mod_config.h"

typedef struct {
  TaskFunction_t fn;
  const char* name;
  configSTACK_DEPTH_TYPE stack_depth;
  UBaseType_t priority;
} Thd_t;

#define THREAD_DECLEAR(_fn, _stack_depth, _priority)                         \
  const Thd_t __comp_init_##_fn __attribute__((section(".thread_init"))) = { \
      .fn = _fn,                                                             \
      .name = #_fn,                                                          \
      .stack_depth = _stack_depth,                                           \
      .priority = _priority,                                                 \
  }

typedef struct {
  /* 机器人状态 */
  struct {
    float battery;
    float vbat;
    float cpu_temp;
  } status;

  Config_t cfg; /* 机器人配置 */

#ifdef MCU_DEBUG_BUILD

#endif
} Runtime_t;

void Thd_Init(void);
