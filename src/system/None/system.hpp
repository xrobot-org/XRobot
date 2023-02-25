#include <cstdint>
#include <database.hpp>
#include <functional>
#include <memory.hpp>
#include <queue.hpp>
#include <semaphore.hpp>
#include <term.hpp>
#include <thread.hpp>
#include <timer.hpp>

#include "om.hpp"

namespace System {
template <typename RobotType, typename... RobotParam>
void Start(RobotParam... param) {
  new Message();
  new Timer();
  new Term();
  new Database();

  RobotType robot(param...);

  Timer::Start();
}
}  // namespace System
