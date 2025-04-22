#include "robot.hpp"

#include <system.hpp>

using namespace Robot;

/* clang-format off */
Robot:: MotorCtrl:: Param param = {
    /* LED引脚和闪烁延时 */
    .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 20,
    },

    .speed_ctrl = {
    /* PID参数 out=k*(p+i+d) */
    .pid = {
        .k = 0.05f,
        .p = 1.5f,
        .i = 0.000f,//0.0156f,
        .d = 0.0f,
        .i_limit = 1.0f,
        .out_limit = 10.0f,
        .d_cutoff_freq = -1.0f,
        .cycle = false,
    },


    /* 电机参数 型号和ID等 */
    .motor = {
        .id_feedback = 0x20A,
        .id_control = GM6020_CTRL_ID_EXTAND,
        .model = Device::RMMotor::MOTOR_GM6020,
        .can = BSP_CAN_1,
    }
}
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::MotorCtrl, Robot::MotorCtrl::Param>(param);
}
