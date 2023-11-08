#include "robot.hpp"

#include <comp_cmd.hpp>
#include <system.hpp>

#include "dev_rm_motor.hpp"

using namespace Robot;

/* clang-format off */
Robot::Engineer::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  },

  .sw_2 = {
    .can = BSP_CAN_2,
    .id = 3,
  },


  .sw_3 = {
    .can = BSP_CAN_2,
    .id = 4,
  },

  .sw_4 = {
    .can = BSP_CAN_2,
    .id = 5,
  },

  .imu = {
    .tp_name_prefix = "custom_ctrl",
    .can = BSP_CAN_2,
    .index = 31,
  },

  .ore_collect = {
    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::OreCollect::RESET
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_TOP,
        Module::OreCollect::FOLD
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_MID,
        Module::OreCollect::WORK
      }
    },

    .x_actr = {
      .limit_param = {
        Device::MicroSwitchLimit::Param{
          .id = 14,
        },
      },
      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0002f,
            .p = 1.0f,
            .i = 0.5f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 120000.0f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 5000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x201,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_2,
            .reverse = true,
        },
      },

      .motor_name = {
        "x_actr"
      },

      .cali_speed = -3000.0f,

      .max_distance = 25.7f * 0.01f,

      .margin_error = 0.0f,

      .zero_position = 0.0f,

      .reduction_ratio = 22.896133278f * 100.0f,

      .axis = Device::AXIS_X
    },

    .pitch_actr = {
      .limit_param = {
        Device::MicroSwitchLimit::Param{
          .id = 19,
        },
      },
      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.5f,
            .d = 0.0f,
            .i_limit = 1.0f,
            .out_limit = 1.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 120000.0f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 5000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x204,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_2,
            .reverse = true,
        },
      },

      .motor_name = {
        "pitch_actr"
      },

      .cali_speed = -3000.0f,

      .min_angle = -66.0f / 180.0f * M_PI,

      .max_angle = 95.0f / 180.0f * M_PI,

      .margin_error = 0.0f,

      .zero_angle = -66.0f / 180.0f * M_PI,

      .reduction_ratio = 19.252851377f / M_PI * 180.0f,

      .translation = {
        .x = 0.0f,
        .y = 0.085f,
        .z = 0.0f,
      },

      .axis = Device::AXIS_X
    },

    .pitch_1_actr = {
      .limit_param = {
        Device::MicroSwitchLimit::Param{
          .id = 15,
        },
      },
      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.5f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 30000.0f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 5000.0f,
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
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_2,
        },
      },

      .motor_name = {
        "pitch_1_actr"
      },

      .cali_speed = -3000.0f,

      .min_angle = -52.8f / 180.0f * M_PI,

      .max_angle = 177.6f / 180.0f * M_PI,

      .margin_error = 0.0f,

      .zero_angle = -52.8f / 180.0f * M_PI,

      .reduction_ratio = 10.07565889f / M_PI * 180.0f,

      .translation = {
        .x = 0.0f,
        .y = 0.2f,
        .z = 0.055f,
      },

      .axis = Device::AXIS_X
    },

    .yaw_actr = {
      .limit_param = {
        Device::MicroSwitchLimit::Param{
          .id = 17,
        },
      },
      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.1f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.8f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 120000.0f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 6000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x203,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_2,
        },
      },

      .motor_name = {
        "yaw_actr"
      },

      .cali_speed = -5000.0f,

      .min_angle = -90.0f / 180.0f * M_PI,

      .max_angle = 90.0f / 180.0f * M_PI,

      .margin_error = 0.0f,

      .zero_angle = -90.0f / 180.0f * M_PI,

      .reduction_ratio = 19.031800126f / M_PI * 180.0f,

      .translation = {
        .x = 0.0f,
        .y = 0.23f,
        .z = 0.0f,
      },

      .axis = Device::AXIS_Z,
    },

    .roll_actr = {
      .limit_param = {
        Device::MicroSwitchLimit::Param{
          .id = 18,
        },
      },
      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.5f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.8f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 3000.0f,
            .p = 1.0f,
            .i = 0.6f,
            .d = 0.0f,
            .i_limit = 300.0f,
            .out_limit = 5000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x202,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_2,
        },
      },

      .motor_name = {
        "roll_actr"
      },

      .cali_speed = 3000.0f,

      .min_angle = -180.0f / 180.0f * M_PI,

      .max_angle = 96.9 / 180.0f * M_PI,

      .margin_error = 0.0f,

      .zero_angle = 96.9 / 180.0f * M_PI,

      .reduction_ratio = 36.0f,

      .translation = {
        .x = 0.0f,
        .y = 0.0f,
        .z = 0.0f,
      },

      .axis = Device::AXIS_Y,
    },

    .y_actr = {
      .limit_param = {
        Device::MicroSwitchLimit::Param{
          .id = 20,
        },
        Device::MicroSwitchLimit::Param{
          .id = 23,
        },
      },
      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.5f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.8f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 120000.0f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 5000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.5f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.8f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 120000.0f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 3000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x208,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_1,
            .reverse = true,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x207,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_2,
            .reverse = false,
        },
      },

      .motor_name = {
        "y_1_actr",
        "y_2_actr"
      },

      .cali_speed = -3000.0f,

      .max_distance = 22.0f * 0.01f,

      .margin_error = 0.0f,

      .zero_position = 0.0f,

      .reduction_ratio = 47.579500315f * 100.0f,

      .axis = Device::AXIS_Y,
    },

    .z_actr = {
      .limit_param = {
        Device::AutoReturnLimit::Param{
          .timeout = 4.0,
        },
        Device::AutoReturnLimit::Param{
          .timeout = 4.0,
        },
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.00015f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.8f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 60000.0f,
            .p = 1.0f,
            .i = 0.1f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 3000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
        Component::PosActuator::Param{
          .speed = {
            .k = 0.00015f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.8f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 60000.0f,
            .p = 1.0f,
            .i = 0.1f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 3000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x208,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_2,
            .reverse = false,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x205,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_2,
            .reverse = true,
        },
      },

      .motor_name = {
        "z_1_1_actr",
        "z_1_2_actr"
      },

      .cali_speed = -0.0f,

      .max_distance = 17.0 * 0.01f,

      .margin_error = 0.0f,

      .zero_position = 0.0f,

      .reduction_ratio = 9.236020649f * 100.0f,

      .axis = Device::AXIS_Z,
    },

    .z_1_actr = {
      .limit_param = {
        Device::AutoReturnLimit::Param{
          .timeout = 1.5,
        },
        Device::AutoReturnLimit::Param{
          .timeout = 1.5,
        },
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.00015f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.8f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 60000.0f,
            .p = 1.0f,
            .i = 0.1f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 3000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        },
        Component::PosActuator::Param{
          .speed = {
            .k = 0.00015f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.8f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 60000.0f,
            .p = 1.0f,
            .i = 0.1f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 3000.0f,
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
            .reverse = true,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x206,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
            .reverse = false,
        },
      },

      .motor_name = {
        "z_2_1_actr",
        "z_2_2_actr"
      },

      .cali_speed = -500.0f,

      .max_distance = 35.0f * 0.01f,

      .margin_error = 0.0f,

      .zero_position = 0.0f,

      .reduction_ratio = 9.923724351f * 100.0f,

      .axis = Device::AXIS_Z,
    },

    .zero_position = {
      .x = -0.13f,
      .y = -0.08f,
      .z = 0.355f,
    },


  },

};
/* clang-format on */
Robot::Engineer* Robot::Engineer::self_;
void robot_init() {
  System::Start<Robot::Engineer, Robot::Engineer::Param>(param, 500.0f);
}
