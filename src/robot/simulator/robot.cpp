#include "robot.hpp"

#include "thread.hpp"

/* clang-format off */

/* clang-format on */

void robot_init() {
  auto init_thread_fn = [](void* arg) {
    RM_UNUSED(arg);

    Robot::Simulator blink;

    while (1) {
      System::Thread::Sleep(UINT32_MAX);
    }
  };

  System::Thread init_thread;

  init_thread.Create(init_thread_fn, (void*)0, "init_thread_fn", 512,
                     System::Thread::Realtime);
}
