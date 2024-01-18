#include <cstdint>
#include <database.hpp>
#include <functional>
#include <memory.hpp>
#include <queue.hpp>
#include <semaphore.hpp>
#include <term.hpp>
#include <thread.hpp>
#include <timer.hpp>

#include "bsp_time.h"
#include "om.hpp"

namespace System {
template <typename RobotType, typename... RobotParam>
void Start(RobotParam... param) {
  new Message();
  new Timer();
  new Term();
  new Database();

  static auto xrobot_debug_handle = new RobotType(param...);

  XB_UNUSED(xrobot_debug_handle);

  uint32_t last_online_time = bsp_time_get_ms();

  while (1) {
    Timer::self_->list_.Foreach(Timer::Refresh, NULL);
    Timer::self_->thread_.SleepUntil(1, last_online_time);
  }
}
}  // namespace System
