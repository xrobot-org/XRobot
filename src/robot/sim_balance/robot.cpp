#include "robot.hpp"

#include <comp_pid.hpp>

#include "system.hpp"

/* clang-format off */
Robot::Simulator::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  },

  .imu = {
    .tp_name_prefix = "imu",
  },

  .chassis={
    .init_g_center = 0.0f,

    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Device::TerminalController::STOP_CTRL,
        Module::RMBalance::SET_MODE_RELAX
      },
      Component::CMD::EventMapItem{
        Device::TerminalController::START_CTRL,
        Module::RMBalance::SET_MODE_INDENPENDENT
      }
    },

    .speed_filter_cutoff_freq = -1.0f,

    .motor_param = {
      Device::RMDMotor::Param{
      },
      Device::RMDMotor::Param{
      },
    },

    .pid_param = {
        /* CTRL_CH_DISPLACEMENT */
        Component::PID::Param{
          .k = 40.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 45.0f,
          .i_limit = 0.6f,
          .out_limit = 0.8f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        /* CTRL_CH_FORWARD_SPEED */
        Component::PID::Param{
          .k = 0.7f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.2f,
          .i_limit = 0.3f,
          .out_limit = 0.6f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        /* CTRL_CH_PITCH_ANGLE */
        Component::PID::Param{
          .k = 3.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },
        /* CTRL_CH_GYRO_X */
        Component::PID::Param{
          .k = 0.4f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        /* CTRL_CH_YAW_ANGLE */
        Component::PID::Param{
          .k = 3.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },
        /* CTRL_CH_GYRO_Z */
        Component::PID::Param{
          .k = 0.03f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.011f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },
      },

      .offset_pid = {
        .k = 1.0f,
        .p = 1.0f,
        .i = 10.0f,
        .d = 0.0f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .cycle = true,
      }
  }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::Simulator, Robot::Simulator::Param>(param);
}
