#pragma once

#include <stdbool.h>
#include <stdint.h>

/* 电源输出接口 */
typedef enum {
  POWER_PORT1,
  POWER_PORT2,
  POWER_PORT3,
  POWER_PORT4,
} bsp_power_port_t;

int8_t bsp_power_set(bsp_power_port_t port, bool s);
