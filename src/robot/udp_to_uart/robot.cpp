#include "robot.hpp"

#include "bsp_uart.h"
#include "system.hpp"

/* clang-format off */
Robot::UdpToUart::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  },

  .topic_share = {
    .topic_name = {"net_info"},
    .block = true,
    .uart = BSP_UART_ESP,
    .cycle = 10,
  }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::UdpToUart, Robot::UdpToUart::Param>(param);
}
