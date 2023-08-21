#include "bsp_time.h"

#include <webots/robot.h>

uint32_t bsp_time_get_ms() { return wb_robot_get_time() * 1000; }

uint64_t bsp_time_get_us() { return wb_robot_get_time() * 1000000; }

uint64_t bsp_time_get() __attribute__((alias("bsp_time_get_us")));
