#pragma once

#include <stdbool.h>
#include <stdint.h>

/* 电源输出接口 */
typedef enum {
  POWER_PORT1,
  POWER_PORT2,
  POWER_PORT3,
  POWER_PORT4,
} BSP_Power_Port_t;

int8_t BSP_Power_Set(BSP_Power_Port_t port, bool s);
