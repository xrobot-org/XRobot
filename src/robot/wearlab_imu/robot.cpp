#include "robot.hpp"

#include "system.hpp"

/* clang-format off */
Robot::WearLabIMU::Param param = {
  .bmi088_rot =
  {
    .rot_mat =
    {
      {+0, -1, +0},
      {+1, +0, +0},
      {+0, +0, +1},
    },
  },

  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::WearLabIMU, Robot::WearLabIMU::Param>(param);
}
