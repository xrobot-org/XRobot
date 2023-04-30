#include "bsp_time.h"

#include "esp_timer.h"

uint32_t bsp_time_get_ms() { return esp_timer_get_time() / 1000; }

uint32_t bsp_time_get_us() { return esp_timer_get_time(); }

float bsp_time_get() { return (float)esp_timer_get_time() / 1000000.0f; }
