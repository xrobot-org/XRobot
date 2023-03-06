#include "robot.hpp"

#include <comp_cmd.hpp>
#include <system.hpp>
#include <thread.hpp>

#include "dev_rm_motor.hpp"

using namespace Robot;

/* clang-format off */
Robot::Engineer::Param param = {
  .led = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  },

  .ore_collect = {
    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::OreCollect::RESET
      },
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::OreCollect::STOP
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_TOP,
        Module::OreCollect::STEP_10
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_MID,
        Module::OreCollect::STEP_8
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_BOT,
        Module::OreCollect::STEP_9
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_TOP,
        Module::OreCollect::STOP
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_MID,
        Module::OreCollect::START
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::OreCollect::STEP_7
      }
    },

    .x_actr = {
      .stall_detect = {
        .speed_thld = 300.0f,
        .current_thld = 2.0f,
        .stop_current_thld = 4.0f,
        .temp_thld = 40.0f,
        .timeout = 0.1f
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0003f,
            .p = 1.0f,
            .i = 0.3f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 600.0f,
            .p = 1.0f,
            .i = 0.1f,
            .d = 0.0f,
            .i_limit = 500.0f,
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
            .id_feedback = 0x201,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_2,
        },
      },

      .motor_name = {
        "x_actr"
      },

      .cali_speed = -2000.0f,

      .max_range = 33.44f,

      .margin_error = 0.5f,

      .reduction_ratio = 3591.0f / 187.0f
    },

    .pitch_actr = {
      .stall_detect = {
        .speed_thld = 300.0f,
        .current_thld = 2.2f,
        .stop_current_thld = 4.0f,
        .temp_thld = 40.0f,
        .timeout = 0.1f
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 3000.0f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 4000.0f,
            .out_limit = 8000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        }
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x204,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_2,
            .reverse = true
        }
      },

      .motor_name = {
        "pitch_1",
      },

      .cali_speed = -2000.0f,

      .max_range = 67.0f,

      .margin_error = 1.0f,

      .reduction_ratio = 36.0f
    },

    .pitch_1_actr = {
      .stall_detect = {
        .speed_thld = 500.0f,
        .current_thld = 2.0f,
        .stop_current_thld = 5.0f,
        .temp_thld = 40.0f,
        .timeout = 0.15f
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.00015f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 1000.0f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 2000.0f,
            .out_limit = 4000.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .in_cutoff_freq = 10.0f,

          .out_cutoff_freq = 30.0f,
        }
      },

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x206,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_2,
            .reverse = false,
        }
      },

      .motor_name = {
        "pitch_2",
      },

      .cali_speed = -2000.0f,

      .max_range = 66.4788589f,

      .margin_error = 0.0f,

      .reduction_ratio = 36.0f
    },

    .yaw_actr = {
      .stall_detect = {
        .speed_thld = 300.0f,
        .current_thld = 2.0f,
        .stop_current_thld = 3.5f,
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
            .i = 0.0f,
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
            .id_feedback = 0x202,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_2,
        },
      },

      .motor_name = {
        "yaw_actr"
      },

      .cali_speed = -4000.0f,

      .max_range = 98.8634644f * 2.0f,

      .margin_error = 1.0f,

      .reduction_ratio = 3591.0f / 187.0f
    },

    .z_actr = {
      .stall_detect = {
        .speed_thld = 300.0f,
        .current_thld = 1.0f,
        .stop_current_thld = 2.0f,
        .temp_thld = 40.0f,
        .timeout = 0.01f
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.6f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 1500.0f,
            .p = 1.0f,
            .i = 0.7f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 2000.0f,
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
            .i = 0.6f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 1500.0f,
            .p = 1.0f,
            .i = 0.7f,
            .d = 0.0f,
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
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_2,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x208,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_2,
            .reverse = true,
        },
      },

      .motor_name = {
        "z_actr_1_1",
        "z_actr_1_2",
      },

      .cali_speed = -500.0f,

      .max_range = 8.45f,

      .margin_error = 0.5f,

      .reduction_ratio = 3591.0f / 187.0f
    },

    .z_1_actr = {
      .stall_detect = {
        .speed_thld = 300.0f,
        .current_thld = 1.0f,
        .stop_current_thld = 2.0f,
        .temp_thld = 40.0f,
        .timeout = 0.01f
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.6f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 1500.0f,
            .p = 1.0f,
            .i = 0.7f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 2000.0f,
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
            .i = 0.6f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 1500.0f,
            .p = 1.0f,
            .i = 0.7f,
            .d = 0.0f,
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
        },
      },

      .motor_name = {
        "z_actr_2_1",
        "z_actr_2_2",
      },

      .cali_speed = -500.0f,

      .max_range = 19.0f,

      .margin_error = 0.5f,

      .reduction_ratio = 3591.0f / 187.0f
    },

    .y_actr = {
      .stall_detect = {
        .speed_thld = 300.0f,
        .current_thld = 1.6f,
        .stop_current_thld = 3.0f,
        .temp_thld = 40.0f,
        .timeout = 0.05f
      },

      .pos_actuator = {
        Component::PosActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.6f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 1500.0f,
            .p = 1.0f,
            .i = 0.7f,
            .d = 0.0f,
            .i_limit = 1000.0f,
            .out_limit = 2000.0f,
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
            .i = 0.6f,
            .d = 0.0f,
            .i_limit = 0.5f,
            .out_limit = 0.5f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
          },

          .position = {
            .k = 1500.0f,
            .p = 1.0f,
            .i = 0.7f,
            .d = 0.0f,
            .i_limit = 2000.0f,
            .out_limit = 000.0f,
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
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_1,
            .reverse = false,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x208,
            .id_control = M3508_M2006_CTRL_ID_EXTAND,
            .model = Device::RMMotor::MOTOR_M2006,
            .can = BSP_CAN_1,
            .reverse = true,
        },
      },

      .motor_name = {
        "y_actr_1",
        "y_actr_2",
      },

      .cali_speed = -1000.0f,

      .max_range = 57.5f,

      .margin_error = 0.5f,

      .reduction_ratio = 3591.0f / 187.0f
    },

  },

};
/* clang-format on */
Robot::Engineer* Robot::Engineer::self_;
void robot_init() {
  System::Start<Robot::Engineer, Robot::Engineer::Param>(param, 500.0f);
}
