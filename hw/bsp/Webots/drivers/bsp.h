#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <webots/robot.h>

#include "bsp_def.h"

typedef struct {
  void (*fn)(void *);
  void *arg;
} bsp_callback_t;

void bsp_init(void);

#ifdef __cplusplus
}
#endif
