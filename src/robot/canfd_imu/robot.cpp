#include "robot.hpp"

#include "system.hpp"

/* clang-format off */
Robot::CanfdIMU::Param param = {
  .icm42688_rot =
  {
    .rot_mat =
    {
        {-1, +0, +0},
        {+0, -1, +0},
        {+0, +0, +1},
    },
  },

  .bmi088_rot =
  {
    .rot_mat =
    {
        {+1, +0, +0},
        {+0, +1, +0},
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
  System::Start<Robot::CanfdIMU, Robot::CanfdIMU::Param>(param);
}
