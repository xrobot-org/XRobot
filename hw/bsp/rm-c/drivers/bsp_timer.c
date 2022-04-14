#include "bsp_timer.h"

#include "comp_utils.h"
#include "hal_tim.h"

inline uint64_t bsp_timer_get_realtime() { return HAL_RealtimeClockGetValue(); }
