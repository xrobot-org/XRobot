#include "robot.hpp"

#include <system.hpp>

using namespace Robot;

/* clang-format off */
Robot:: MotorCtrl:: Param param = {
    /* LED引脚和闪烁延时 */
    .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
    },

    .speed_ctrl = {
    /* PID参数 out=k*(p+i+d) */
    .pid = {
        .k = 0.5f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .cycle = false,
    },

    /* 电机参数 型号和ID等 */
    .motor = {
        .id_feedback = 0x204,
        .id_control = M3508_M2006_CTRL_ID_BASE,
        .model = Device::RMMotor::MOTOR_M2006,
        .can = BSP_CAN_1,
    }
    }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::MotorCtrl, Robot::MotorCtrl::Param>(param);
}
