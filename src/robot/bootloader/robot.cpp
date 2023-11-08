#include "robot.hpp"

#include <system.hpp>

using namespace Robot;

/* clang-format off */
Robot::Bootloader::Param param ={
  .uart_update = {
    .timeout = 3000,
    .board_id = 0x01,
  }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::Bootloader, Robot::Bootloader::Param>(param);
}
