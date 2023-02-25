#include "robot.hpp"

#include "system.hpp"

/* clang-format off */
Robot::Blink::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  }
};
/* clang-format on */

void robot_init() { System::Start<Robot::Blink, Robot::Blink::Param>(param); }
