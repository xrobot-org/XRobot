#pragma once

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
