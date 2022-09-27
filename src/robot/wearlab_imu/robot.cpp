#include "robot.hpp"

/* clang-format off */
Robot::Infantry::Param param = {
  .bmi088_rot =
  {
    .rot_mat =
    {
      {-1, +0, +0},
      {+0, +1, +0},
      {+0, +0, -1},
    },
  },

  .bmi088_cali =
  {
    .gyro_offset =
    {
      .x = -0.00174048,
      .y = -0.00141542,
      .z = +0.00311609,
    },
  },
};
/* clang-format on */

void robot_init() {
  auto init_thread_fn = [](void* arg) {
    RM_UNUSED(arg);

    Robot::Infantry robot(param);

    while (1) {
      System::Thread::Sleep(UINT32_MAX);
    }
  };

  System::Thread init_thread;
  THREAD_DECLEAR(init_thread, init_thread_fn, 512, System::Thread::Realtime,
                 NULL);
}
