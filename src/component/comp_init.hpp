#pragma once

#include "comp_type.hpp"

typedef struct {
  const char* fname;
  err_t (*fn)(const void* param);
  const void* param;
} init_t;

void init(void);

#define INIT_DECLEAR(fn, param, stage)                 \
  const init_t __comp_init_##fn                        \
      __attribute__((section(".init_fn." #stage))) = { \
          .name = #fn,                                 \
          .fn = fn,                                    \
          .param = param,                              \
  }

#define INIT_HAL_DECLEAR(fn) INIT_DECLEAR(fn, 1)
#define INIT_BSP_EARLY_DECLEAR(fn) INIT_DECLEAR(fn, 2)
#define INIT_BSP_MIDDLE_DECLEAR(fn) INIT_DECLEAR(fn, 3)
#define INIT_BSP_FINAL_DECLEAR(fn) INIT_DECLEAR(fn, 4)

#define INIT_START_ADDRESS(stage) __init_##stage##_start
#define INIT_HAL_START_ADDRESS INIT_START_ADDRESS(1)
#define INIT_BSP_EARLY_START_ADDRESS INIT_START_ADDRESS(2)
#define INIT_BSP_MIDDLE_START_ADDRESS INIT_START_ADDRESS(3)
#define INIT_BSP_FINAL_START_ADDRESS INIT_START_ADDRESS(4)

#define INIT_END_ADDRESS(stage) __init_##stage##_end
#define INIT_HAL_END_ADDRESS INIT_END_ADDRESS(1)
#define INIT_BSP_EARLY_END_ADDRESS INIT_END_ADDRESS(2)
#define INIT_BSP_MIDDLE_END_ADDRESS INIT_END_ADDRESS(3)
#define INIT_BSP_FINAL_END_ADDRESS INIT_END_ADDRESS(4)
