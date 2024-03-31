#include "robot.hpp"

#include <cmath>
#include <system.hpp>
#include <thread.hpp>

#include "dev_rm_motor.hpp"

using namespace Robot;

/* clang-format off */
Robot::UVA::Param param={
  .gimbal{
     .yaw_actr = {
      .speed = {
          /* GIMBAL_CTRL_YAW_OMEGA_IDX */
          .k = 0.2f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.2f,
          .out_limit = 0.1f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_YAW_ANGLE_IDX */
          .k = 18.0f,
          .p = 2.5f,
          .i = 0.0f,
          .d = 0.0f,
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
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_PIT_ANGLE_IDX */
          .k = 20.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
    },

    .yaw_motor = {
      .id_feedback = 0x207,
      .id_control = GM6020_CTRL_ID_BASE,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_2,
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
      .yaw = 4.8f,
      .pit = 1.7f,
      .rol = 0.0f,
    },

    .limit = {
      .pitch_max = M_2PI - 0.06,
      .pitch_min = M_2PI - 1.08,
      .yaw_max = M_2PI - 3.8,
      .yaw_min = M_2PI - 5.5,

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
  .trig_gear_ratio = 36.0f,
  .num_trig_tooth = 8.0f,
  .min_launch_delay = static_cast<uint32_t>(1000.0f / 20.0f),

  .trig_actr = {
      Component::PosActuator::Param{
        .speed = {
          .k = 1.5,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.03f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 1.2f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.012f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },

    .trig_motor = {
      Device::RMMotor::Param{
        .id_feedback = 0x203,
        .id_control = M3508_M2006_CTRL_ID_BASE,
        .model = Device::RMMotor::MOTOR_M2006,
        .can = BSP_CAN_2,
      }
    },
    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::UVALauncher::CHANGE_FIRE_MODE_SAFE
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_TOP,
        Module::UVALauncher::CHANGE_FIRE_MODE_SAFE
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_MID,
        Module::UVALauncher::CHANGE_TRIG_MODE_SINGLE
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::UVALauncher::CHANGE_TRIG_MODE_BURST
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::UVALauncher::LAUNCHER_START_FIRE
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_L_PRESS,
        Module::UVALauncher::CHANGE_TRIG_MODE_SINGLE
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_L_RELEASE,
        Module::UVALauncher::CHANGE_FIRE_MODE_SAFE
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_R,
        Module::UVALauncher::OPEN_COVER
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_F,
        Module::UVALauncher::CLOSE_COVER
      }
    },
  },
 .bmi088_rot = {
    .rot_mat = {
      { +1, +0, +0},
      { +0, +1, +0},
      { +0, +0, +1},
    },
  },
};
/* clang-format on */
Robot::UVA *Robot::UVA::self_;

void robot_init() {
  System::Start<Robot::UVA, Robot::UVA::Param>(param, 500.0f);
}
