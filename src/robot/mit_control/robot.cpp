#include "robot.hpp"

#include <system.hpp>
#include <thread.hpp>

using namespace Robot;

/* clang-format off */
//TODO: write your param
/* clang-format on */
Robot::MitControl::Param param = {
  .mitcontrol={
    /*电机型号 参数 ID*/
    .mitmotor_1={
      .kp = 0,
      .kd = 0,
      .feedback_id = 0x00,
      .id = 0x05,
      .can = BSP_CAN_1,
      .reverse = false,
    },
    .mitmotor_2 = {
      .kp = 0,
      .kd = 0,
      .feedback_id = 0x00,
      .id = 0x04,
      .can = BSP_CAN_1,
      .reverse = false,
    }
  }
};

void robot_init() {
  System::Start<Robot::MitControl, Robot::MitControl::Param>(param);
}