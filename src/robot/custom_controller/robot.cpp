#include "robot.hpp"

#include <system.hpp>

using namespace Robot;

/* clang-format off */
Robot::CustomController::Param param;
/* clang-format on */

void robot_init() {
  System::Start<Robot::CustomController, Robot::CustomController::Param>(param);
}
