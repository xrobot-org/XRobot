#include "robot.hpp"

#include <system.hpp>
#include <thread.hpp>

Robot::CustomController::Param param = {
    .led =
        {
            .gpio = BSP_GPIO_LED,
            .timeout = 200,
        },

    .custom_ctrl = {.config_ = {
                        Device::MA600::Param{BSP_SPI1_CS_1, 10},
                        Device::MA600::Param{BSP_SPI1_CS_2, 10},
                        Device::MA600::Param{BSP_SPI1_CS_3, 10},
                        Device::MA600::Param{BSP_SPI2_CS_1, 10},
                        Device::MA600::Param{BSP_SPI2_CS_2, 10},
                        Device::MA600::Param{BSP_SPI2_CS_3, 10},
                    }}};

/* clang-format off */
//TODO: write your param
/* clang-format on */

void robot_init() {
  new System::Timer();

  static auto xrobot_debug_handle = new Robot::CustomController(param);

  XB_UNUSED(xrobot_debug_handle);

  uint32_t last_online_time = bsp_time_get_ms();

  while (1) {
    System::Timer::self_->list_.Foreach(System::Timer::Refresh, NULL);
    System::Timer::self_->thread_.SleepUntil(1, last_online_time);
  }
}
