#include "robot.hpp"

#include <system.hpp>

using namespace Robot;

/* clang-format off */
Robot::CanToUart::Param param;
/* clang-format on */

void robot_init() {
  System::Start<Robot::CanToUart, Robot::CanToUart::Param>(param);
}
