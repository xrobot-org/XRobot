#include "robot.hpp"

#include <system.hpp>
#include <thread.hpp>

using namespace Robot;

/* clang-format off */
//TODO: write your param
Robot::Drone::Param param={

  .gimbal{
        .ff = {
      /* GIMBAL_CTRL_PIT_FEEDFORWARD */
      .a = 0.0439f,
      .b = -0.0896f,
      .c = 0.077f,
      .max = 0.1f,
      .min = -0.2f,
    }, /* ff */

    .st = {
      /* GIMBAL_CTRL_YAW_SELF_TUNING */
      .a = 0.0677f,
      .b = 0.1653f,
      .c = 0.3379f,
      .max = 0.37f,
      .min = 0.29f,
    }, /* st */

     .yaw_actr = {
      .speed = {
          /* GIMBAL_CTRL_YAW_OMEGA_IDX */
          .k = 0.2f,
          .p = 0.8f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.2f,
          .out_limit = 0.1f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_YAW_ANGLE_IDX */
          .k = 18.0f,
          .p = 3.6f,
          .i = 0.1f,
          .d = 0.05f,
          .i_limit = 0.1f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
    },
    .pit_actr = {
        .speed = {
          /* GIMBAL_CTRL_PIT_OMEGA_IDX */
          .k = 0.25f,
          .p = 0.8f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_PIT_ANGLE_IDX */
          .k = 10.0f,
          .p = 0.5f,
          .i = 0.0f,
          .d = 0.05f,
          .i_limit = 1.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
    },

         .yaw_ai_actr = {
      .speed = {
          /* GIMBAL_CTRL_YAW_OMEGA_IDX */
          .k = 0.2f,
          .p = 0.8f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.2f,
          .out_limit = 0.1f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_YAW_ANGLE_IDX */
          .k = 18.0f,
          .p = 3.6f,
          .i = 0.1f,
          .d = 0.05f,
          .i_limit = 0.1f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
    },
    .pit_ai_actr = {
        .speed = {
          /* GIMBAL_CTRL_PIT_OMEGA_IDX */
          .k = 0.25f,
          .p = 0.8f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_PIT_ANGLE_IDX */
          .k = 10.0f,
          .p = 0.5f,
          .i = 0.0f,
          .d = 0.05f,
          .i_limit = 1.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
    },

    .yaw_motor = {
      .id_feedback = 0x208,
      .id_control = GM6020_CTRL_ID_BASE,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_1,
      .reverse = true
    },

    .pit_motor = {
      .id_feedback = 0x205,
      .id_control = GM6020_CTRL_ID_BASE,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_2,
      .reverse = true
    },

    .mech_zero = {
      .yaw = 0.0f,
      .pit = 0.f,
      .rol = 0.0f,
    },

    .limit = {
      .pitch_max = M_2PI-5.97f,
      .pitch_min = M_2PI-1.f,
      .yaw_max = 5.5f,
      .yaw_min = 3.5f,
    },

    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::Gimbal::SET_MODE_RELAX
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_TOP,
        Module::Gimbal::SET_MODE_RELAX
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_MID,
        Module::Gimbal::SET_MODE_ABSOLUTE
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_BOT,
        Module::Gimbal::SET_MODE_ABSOLUTE
      }
    }
  },
  .launcher={
    .trig_gear_ratio=36.0f,
    .bullet_circle_num=8.0f,
    .min_launcher_delay= static_cast<uint32_t>(1000.0f/20.0f),
    .trig_actr={
      Component::PosActuator::Param{
        .speed = {
          .k = 3.0f,
          .p = 1.0f,
          .i = 0.5f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 1.5f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },
    .trig_motor={
      Device::RMMotor::Param{
        .id_feedback = 0x203,
        .id_control = M3508_M2006_CTRL_ID_BASE,
        .model = Device::RMMotor::MOTOR_M2006,
        .can = BSP_CAN_1,
        .reverse=false,
      }
    },
    .EVENT_MAP={
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::DroneLauncher::SET_RELAX,
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_TOP,
        Module::DroneLauncher::SET_RELAX,
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_MID,
        Module::DroneLauncher::CHANGE_FRIC_MODE_LOADED,
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::DroneLauncher::CHANGE_TRIG_MODE_CONTINUED,
      },
    }
  },
  .bmi088_rot={
    .rot_mat={
      { +1, +0, +0},
      { +0, +1, +0},
      { +0, +0, +1},
    },
  },
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::Drone, Robot::Drone::Param>(param, 500.0f);
}
