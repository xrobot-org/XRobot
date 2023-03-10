#include "robot.hpp"

#include <comp_actuator.hpp>
#include <comp_pid.hpp>

#include "dev_mit_motor.hpp"
#include "system.hpp"

/* clang-format off */
Robot::Infantry::Param param = {
    .balance = {
      .init_g_center = 0.08f,

      .follow_pid_param = {
        .k = 0.2f,
        .p = 1.0f,
        .i = 0.5f,
        .d = 0.0f,
        .i_limit = 0.011f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .cycle = true,
      },

      .comp_pid_param = {
        .k = 0.2f,
        .p = 1.0f,
        .i = 0.5f,
        .d = 0.0f,
        .i_limit = 0.011f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .cycle = true,
      },

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
          Module::RMBalance::SET_MODE_FOLLOW
        }
      },

      .wheel_param = {
        Component::SpeedActuator::Param{
          .speed = {
            .k = 0.0002f,
            .p = 1.0f,
            .i = 1.0f,
            .d = 0.0f,
            .i_limit = 1.0f,
            .out_limit = 1.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = -1.0f,

          .out_cutoff_freq = -1.0f,
        },
        Component::SpeedActuator::Param{
          .speed = {
            .k = 0.0002f,
            .p = 1.0f,
            .i = 1.0f,
            .d = 0.0f,
            .i_limit = 1.0f,
            .out_limit = 1.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = -1.0f,

          .out_cutoff_freq = -1.0f,
        },
      },

      .eulr_param = Component::PID::Param{
        .k = 2.0f,
        .p = 1.0f,
        .i = 1.0f,
        .d = 0.04f,
        .i_limit = 20.0f,
        .out_limit = 20.0f,
        .d_cutoff_freq = -1.0f,
        .cycle = true,
      },

      .gyro_param = Component::PID::Param{
        .k = 0.2f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .cycle = false,
      },

      .speed_param = Component::PID::Param{
          .k = 2.0f,
          .p = 1.0f,
          .i = 1.0f,
          .d = 0.0f,
          .i_limit = 0.15f,
          .out_limit = 0.2f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
      },

      .center_filter_cutoff_freq = 10.0f,

      .motor_param = {
        Device::RMMotor::Param{
          .id_feedback = 0x202,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
        },
        Device::RMMotor::Param{
          .id_feedback = 0x201,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
        },
      },

    },

    .leg = {
      .l1 = 0.11f,
      .l2 = 0.2f,
      .l3 = 0.31f,
      .l4 = 0.455f,

      .limit = {
        .high_max = 0.45,
        .high_min = 0.15,
      },

      .leg_max_angle = 0.0f,

      .motor_zero = {
        0.630508065f,
        5.75973034f,
        1.45487654f,
        3.00471401f,
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
          Module::WheelLeg::SET_MODE_SQUAT
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_BOT,
          Module::WheelLeg::SET_MODE_SQUAT
        }
      },

      .leg_actr = {
        Component::PosActuator::Param{
        .speed = {
          .k = 1.0f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 3.0f,
          .out_limit = 3.0f,
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
          .i_limit = 3.0f,
          .out_limit = 3.0f,
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
          .i_limit = 3.0f,
          .out_limit = 3.0f,
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
          .i_limit = 3.0f,
          .out_limit = 3.0f,
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
          .kp = 30.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 1,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
        Device::MitMotor::Param{
          .kp = 30.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 2,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
        Device::MitMotor::Param{
          .kp = 30.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 3,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
        Device::MitMotor::Param{
          .kp = 30.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 4,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
      },
  },

  .bmi088_rot = {
    .rot_mat = {
      { +1, +0, +0},
      { +0, +1, +0},
      { +0, +0, +1},
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
