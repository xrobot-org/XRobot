#include "robot.hpp"

#include <system.hpp>
#include <thread.hpp>

using namespace Robot;

/* clang-format off */
MSG::Param param = {
  .msg_test = {

  }
};
/* clang-format on */

void robot_init() {
  auto init_thread_fn = [](void* arg) {
    static_cast<void>(arg);

    System::Init();

    MSG robot(param);

    while (1) {
      System::Thread::Sleep(UINT32_MAX);
    }
  };

  System::Thread init_thread;

  init_thread.Create(init_thread_fn, static_cast<void*>(NULL), "init_thread_fn",
                     512, System::Thread::REALTIME);
}
