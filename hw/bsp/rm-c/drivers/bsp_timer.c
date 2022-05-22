#include "bsp_timer.h"

#include "main.h"

inline uint64_t bsp_timer_get_realtime() { return HAL_RealtimeClockGetValue(); }
