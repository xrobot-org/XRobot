#include "robot.hpp"

#include "system.hpp"

/* clang-format off */
Robot::Infantry::Param param = {
    .balance = {
      .init_g_center = 0.0f,

      .EVENT_MAP = {
        Component::CMD::EventMapItem{
          Component::CMD::CMD_EVENT_LOST_CTRL,
          Module::RMBalance::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_TOP,
          Module::RMBalance::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_MID,
          Module::RMBalance::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_BOT,
          Module::RMBalance::SET_MODE_INDENPENDENT
        }
      },

      .speed_filter_cutoff_freq = 0.0f,

      .motor_param = {
        Device::RMDMotor::Param{
          .num = 0,
          .can = BSP_CAN_2,
          .reverse = false,
        },
        Device::RMDMotor::Param{
          .num = 1,
          .can = BSP_CAN_2,
          .reverse = false,
        },
      },

      .pid_param = {
        /* CTRL_CH_DISPLACEMENT */
        Component::PID::Param{
          .k = 0.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 15.0f,
          .i_limit = 0.0f,
          .out_limit = 0.35f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        /* CTRL_CH_FORWARD_SPEED */
        Component::PID::Param{
          .k = 0.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        /* CTRL_CH_PITCH_ANGLE */
        Component::PID::Param{
          .k = 1.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.1f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },
        /* CTRL_CH_GYRO_X */
        Component::PID::Param{
          .k = 0.2f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.008f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        /* CTRL_CH_YAW_ANGLE */
        Component::PID::Param{
          .k = 0.05f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.011f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },
        /* CTRL_CH_GYRO_Z */
        Component::PID::Param{
          .k = 0.05f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.011f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },
      }

    },

    .leg = {
      .l1 = 0.11f,
      .l2 = 0.2f,
      .l3 = 0.31f,
      .l4 = 0.455f,

      .limit = {
        .high_max = 0.45,
        .high_min = 0.16,
      },

      .leg_max_angle = 0.0f,

      .motor_zero = {
        5.87863111f,
        3.565917625f,
        1.73f,
        2.86f,
      },

      .EVENT_MAP = {
        Component::CMD::EventMapItem{
          Component::CMD::CMD_EVENT_LOST_CTRL,
          Module::WheelLeg::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_TOP,
          Module::WheelLeg::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_MID,
          Module::WheelLeg::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_BOT,
          Module::WheelLeg::SET_MODE_RELAX
        }
      },

      .leg_actr = {
        Component::PosActuator::Param{
        .speed = {
          .k = 1.0f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 2.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 3.0f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = 250.0f,
        },
        Component::PosActuator::Param{
        .speed = {
          .k = 1.0f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 2.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 3.0f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = 250.0f,
        },
        Component::PosActuator::Param{
        .speed = {
          .k = 1.0f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 2.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 3.0f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = 250.0f,
        },
        Component::PosActuator::Param{
        .speed = {
          .k = 1.0f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 2.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 3.0f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = 250.0f,
        },
      },

      .leg_motor = {
        Device::MitMotor::Param{
          .kp = 60.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 1,
          .can = BSP_CAN_2,
          .max_error = 0.05f,
        },
        Device::MitMotor::Param{
          .kp = 60.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 2,
          .can = BSP_CAN_2,
          .max_error = 0.05f,
        },
        Device::MitMotor::Param{
          .kp = 60.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 1,
          .can = BSP_CAN_1,
          .max_error = 0.05f,
        },
        Device::MitMotor::Param{
          .kp = 60.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 4,
          .can = BSP_CAN_1,
          .max_error = 0.05f,
        },
      },
  },

  .bmi088_rot = {
    .rot_mat = {
      {+0, -1, +0},
      {+1, +0, +0},
      {+0, +0, +1},
    },
  },

  .cap = {
    .can = BSP_CAN_1,
    .index = 0x30,
  },

  .can_imu = {
    .tp_name_prefix = "chassis",

    .can = BSP_CAN_1,

    .index = 30,
  }
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::Infantry, Robot::Infantry::Param>(param, 500.0f);
}
