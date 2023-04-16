#include "robot.hpp"

#include "system.hpp"

/* clang-format off */
Robot::UdpToUart::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  },

  .uart_udp = {
    .port = 4321,
  }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::UdpToUart, Robot::UdpToUart::Param>(param);
}
