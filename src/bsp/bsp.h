#pragma once

typedef struct {
  void (*Fn)(void *);
  void *arg;
} BSP_Callback_t;

#define BSP_OK (0)
#define BSP_ERR (-1)
#define BSP_ERR_NULL (-2)
#define BSP_ERR_INITED (-3)
#define BSP_ERR_NO_DEV (-4)

#define SIGNAL_BSP_USB_BUF_RECV (1u << 0)
