#include "robot.hpp"

#include <system.hpp>

using namespace Robot;

/* clang-format off */
Robot::MicroSwitch::Param param{
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  }
  };
/* clang-format on */

void robot_init() {
  System::Start<Robot::MicroSwitch, Robot::MicroSwitch::Param>(param);
}
