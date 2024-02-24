#include "robot.hpp"

#include <system.hpp>

#include "comp_actuator.hpp"
#include "dev_rm_motor.hpp"

using namespace Robot;

/* clang-format off */
Dart::Param param = {
  .dart = {
    .rod_actr = {
      .stall_detect = {
        .speed_thld = 500.0f,
        .current_thld = 1.2f,
        .stop_current_thld = 2.5f,
        .temp_thld = 40.0f,
        .timeout = 0.1f
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.00005f,
            .p = 1.0f,
            .i = 0.8f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 2000.0f,
            .p = 1.0f,
            .i = 0.6f,
            .d = 0.0f,
            .i_limit = 4000.0f,
            .out_limit = 8000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x205,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
        },
      },

      .motor_name = {
        "rod_actr"
      },

      .cali_speed = -2000.0f,

      .max_range = 322.0f,

      .margin_error = 3.0f,

      .reduction_ratio = 3591.0f / 187.0f
    },
    .reload_actr = {
      .stall_detect = {
        .speed_thld = 0.0f,
        .current_thld = 0.0f,
        .stop_current_thld = 0.0f,
        .temp_thld = -1.0f,
        .timeout = 0.0f
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0002f,
            .p = 4.0f,
            .i = 0.6f,
            .d = 0.03f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 6000.0f,
            .p = 2.0f,
            .i = 1.0f,
            .d = 0.8f,
            .i_limit = 1000.0f,
            .out_limit = 2000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x207,
            .id_control = GM6020_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_GM6020,
            .can = BSP_CAN_1,
        },
      },

      .motor_name = {
        "reload"
      },

      .cali_speed = -0.0f,

      .max_range = 6.29748f,

      .margin_error = 0.0f,

      .reduction_ratio = 3591.0f / 187.0f
    },

    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::DartLauncher::OFF
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_TOP,
        Module::DartLauncher::OFF
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_MID,
        Module::DartLauncher::ON
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_TOP,
        Module::DartLauncher::RESET
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::DartLauncher::RELOAD
      },
    },

    .fric_actr = {
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.8f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.8f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.8f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.8f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
      }
    },

    .fric_motor = {
        Device::RMMotor::Param{
            .id_feedback = 0x201,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
            .reverse = false,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x202,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
            .reverse = true ,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x203,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
            .reverse = true,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x204,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
            .reverse = false,
        }
    }
  },
    .gimbal={
      .yaw_actr = {
        .speed = {
            /* GIMBAL_CTRL_YAW_OMEGA_IDX */
            .k = 0.001f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 0.2f,
            .out_limit = 1.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            /* GIMBAL_CTRL_YAW_ANGLE_IDX */
            .k = 100.0f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 0.0f,
            .out_limit = 1000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = -1.0f,

          .out_cutoff_freq = -1.0f,
    },

    .pitch_actr = {
      .stall_detect = {
        .speed_thld = 1300.0f,
        .current_thld = 2.0f,
        .stop_current_thld = 4.0f,
        .temp_thld = 48.0f,
        .timeout = 0.1f
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0002f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 500.0f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.8f,
            .i_limit = 1000.0f,
            .out_limit = 2000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x206,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_2,
        },
      },

      .motor_name = {
        "pitch"
      },

      .cali_speed = -2000.0f,

      .max_range = 30.0f,

      .margin_error = 1.0f,

      .reduction_ratio = 36
    },

    .yaw_motor = {
      .id_feedback = 0x20A,
      .id_control = GM6020_CTRL_ID_EXTAND,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_2,
      .reverse = false,
    },

    .yaw_zero = 0.7432,
  },
  .bmi088 = {
    .rot_mat = {
      { +1, +0, +0},
      { +0, +1, +0},
      { +0, +0, +1},
    },
  }
};

/* clang-format on */

void robot_init() {
  System::Start<Robot::Dart, Robot::Dart::Param>(param, 500.0f);
}
