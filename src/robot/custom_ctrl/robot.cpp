#include "robot.hpp"

#include "system.hpp"
#include "timer.hpp"

/* clang-format off */
Robot::CustomController::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  },
  .custom_ctrl = {
    .mt6701 = {
      {
        .address = 0x06,
        .i2c = BSP_I2C_1,
        .mech_zero = 0.0f,
        .timeout = 5,
      },{
        .address = 0x06,
        .i2c = BSP_I2C_2,
        .mech_zero = 0.0f,
        .timeout = 5,
      },{
        .address = 0x06,
        .i2c = BSP_I2C_3,
        .mech_zero = 0.0f,
        .timeout = 5,
      },{
        .address = 0x06,
        .i2c = BSP_I2C_4,
        .mech_zero = 0.0f,
        .timeout = 5,
      },{
        .address = 0x06,
        .i2c = BSP_I2C_5,
        .mech_zero = 0.0f,
        .timeout = 5,
      },{
        .address = 0x06,
        .i2c = BSP_I2C_6,
        .mech_zero = 0.0f,
        .timeout = 5,
      },
    }
  }
};
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
