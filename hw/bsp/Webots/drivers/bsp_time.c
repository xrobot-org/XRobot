#include "bsp_time.h"

#include <webots/robot.h>

uint32_t bsp_time_get_ms() { return wb_robot_get_time() * 1000; }

uint32_t bsp_time_get_us() { return wb_robot_get_time() * 1000000; }

float bsp_time_get() { return wb_robot_get_time(); }
