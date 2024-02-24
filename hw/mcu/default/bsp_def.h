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

#define XB_TO_STR(_arg) #_arg
#define XB_DEF2STR(_arg) XB_TO_STR(_arg)

__attribute__((unused)) static void xb_verify_failed(const char* file,
                                                     uint32_t line) {
  while (1) {
    while (1) {
      printf("Assert error at %s:%d\r\n", file, line);
    }
  }
}

#if MCU_DEBUG_BUILD
#define XB_ASSERT(arg)                    \
  if (!(arg)) {                           \
    xb_verify_failed(__FILE__, __LINE__); \
  }
#else
#define XB_ASSERT(arg) (void)0;
#endif

#define XB_UNUSED(_x) ((void)(_x))

#define XB_OFFSET_OF(type, member) ((size_t) & ((type*)0)->member)

#define XB_MEMBER_SIZE_OF(type, member) (sizeof(typeof(((type*)0)->member)))

#define XB_CONTAINER_OF(ptr, type, member)               \
  ({                                                     \
    const typeof(((type*)0)->member)* __mptr = (ptr);    \
    (type*)((char*)__mptr - ms_offset_of(type, member)); \
  })

typedef enum {
  BSP_OK,
  BSP_ERR,
  BSP_ERR_NULL,
  BSP_ERR_INITED,
  BSP_ERR_NO_DEV,
  BSP_ERR_BUSY,
  BSP_ERR_TIMEOUT,
  BSP_ERR_FULL,
  BSP_ERR_EMPTY,
} bsp_status_t;

#ifdef __cplusplus
}
#endif
