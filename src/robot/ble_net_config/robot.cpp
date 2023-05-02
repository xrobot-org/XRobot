#include "robot.hpp"

#include "system.hpp"

/* clang-format off */
Robot::NetConfig::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  },

  .topic_share = {
    .topic_name = "net_info",
    .block = false,
    .uart = BSP_UART_MCU,
    .cycle = 200,
  }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::NetConfig, Robot::NetConfig::Param>(param);
}
