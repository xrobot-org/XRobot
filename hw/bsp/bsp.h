#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  void (*fn)(void *);
  void *arg;
} bsp_callback_t;

#define BSP_OK (0)
#define BSP_ERR (-1)
#define BSP_ERR_NULL (-2)
#define BSP_ERR_INITED (-3)
#define BSP_ERR_NO_DEV (-4)

void bsp_init(void);

#ifdef __cplusplus
}
#endif
