#include <webots/robot.h>

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
  auto init_fun = [](RobotParam... param) {
    new Message();
    new Term();
    new Database();
    new Timer();

    if (wb_robot_get_basic_time_step() >= 2.0f) {
      OMLOG_WARNING(
          "webots basic time step should be less than 2ms, but now it is "
          "%.3f ms.",
          wb_robot_get_basic_time_step());
    }
    static auto xrobot_debug_handle = new RobotType(param...);

    XB_UNUSED(xrobot_debug_handle);

    while (1) {
      poll(NULL, 0, UINT32_MAX);
    }
  };

  std::function<void(void)>* init_fun_call =
      new std::function<void(void)>(std::bind(init_fun, param...));

  auto init_thread_fn = [](std::function<void(void)>* init_fun) {
    (*init_fun)();
  };

  System::Thread init_thread;

  init_thread.Create(init_thread_fn, init_fun_call, "init_thread_fn",
                     INIT_TASK_STACK_DEPTH, System::Thread::REALTIME);
}
}  // namespace System
