#include "robot.hpp"

#include <system.hpp>

using namespace Robot;

/* clang-format off */
CanFdToUart::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::CanFdToUart, Robot::CanFdToUart::Param>(param);
}
