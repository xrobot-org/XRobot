#include "robot.hpp"

#include <comp_actuator.hpp>

#include "bsp_can.h"
#include "dev_rm_motor.hpp"
#include "mod_chassis.hpp"
#include "system.hpp"

/* clang-format off */
Robot::Hero::Param param = {
    .chassis={
      .type = Component::Mixer::MECANUM,

      .follow_pid_param = {
      .k = 0.5f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.0f,
      .i_limit = 1.0f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -1.0f,
      .cycle = true,
      },

      .EVENT_MAP = {
        Component::CMD::EventMapItem{
          Component::CMD::CMD_EVENT_LOST_CTRL,
          Module::RMChassis::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_TOP,
          Module::RMChassis::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_MID,
          Module::RMChassis::SET_MODE_FOLLOW
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_BOT,
          Module::RMChassis::SET_MODE_ROTOR
        },
        Component::CMD::EventMapItem{
          Device::DR16::KEY_V,
          Module::RMChassis::SET_MODE_ROTOR
        },
        Component::CMD::EventMapItem{
          Device::DR16::KEY_B,
          Module::RMChassis::SET_MODE_FOLLOW
        }
      },

      .actuator_param = {
        Component::SpeedActuator::Param{
          .speed = {
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.0f,
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
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.0f,
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
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.0f,
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
            .k = 0.0001f,
            .p = 1.0f,
            .i = 0.0f,
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

      .motor_param = {
        Device::RMMotor::Param{
            .id_feedback = 0x201,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x202,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x203,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
        },
        Device::RMMotor::Param{
            .id_feedback = 0x204,
            .id_control = M3508_M2006_CTRL_ID_BASE,
            .model = Device::RMMotor::MOTOR_M3508,
            .can = BSP_CAN_1,
        },
    },
  },


  .gimbal = {
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
          .k = 0.85f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.8f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_YAW_ANGLE_IDX */
          .k = 20.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.0f,
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
          .k = 0.5f,
          .p = 1.0f,
          .i = 0.6f,
          .d = 0.f,
          .i_limit = 0.8f,
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
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
    },

    .yaw_motor = {
      .id_feedback = 0x209,
      .id_control = GM6020_CTRL_ID_EXTAND,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_2,
      .reverse = true,
    },

    .pit_motor = {
      .id_feedback = 0x20A,
      .id_control = GM6020_CTRL_ID_EXTAND,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_2,
      .reverse = true,
    },

    .mech_zero = {
      .yaw = 1.3f,
      .pit = 4.0f,
      .rol = 0.0f,
    },

    .limit = {
      .pitch_max = 2.7f,
      .pitch_min = 2.0f,
    },

    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::Gimbal::SET_MODE_RELAX
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_TOP,
        Module::Gimbal::SET_MODE_ABSOLUTE
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_MID,
        Module::Gimbal::SET_MODE_ABSOLUTE
        },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::Gimbal::SET_MODE_ABSOLUTE
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_R_PRESS,
        Module::Gimbal::START_AUTO_AIM
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_R_RELEASE,
        Module::Gimbal::STOP_AUTO_AIM
      }
    },

  },

  .launcher = {
    .num_trig_tooth = 6.0f,
    .trig_gear_ratio = 3591.0f / 187.0f,
    .fric_radius = 0.03f,
    .cover_open_duty = 0.125f,
    .cover_close_duty = 0.075f,
    .model = Module::Launcher::LAUNCHER_MODEL_42MM,
    .default_bullet_speed = 16.0f,
    .min_launch_delay = 800,

    .trig_actr = {
      Component::PosActuator::Param{
        .speed = {
          .k = 10.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 0.8f,
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

    .fric_actr = {
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.001f,
          .p = 1.0f,
          .i = 0.2,
          .d = 0.0f,
          .i_limit = 0.3f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.001f,
          .p = 1.0f,
          .i = 0.2f,
          .d = 0.0f,
          .i_limit = 0.3f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },

    .trig_motor = {
      Device::RMMotor::Param{
        .id_feedback = 0x207,
        .id_control = M3508_M2006_CTRL_ID_EXTAND,
        .model = Device::RMMotor::MOTOR_M2006,
        .can = BSP_CAN_1,
      }
    },

    .fric_motor = {
      Device::RMMotor::Param{
          .id_feedback = 0x206,
          .id_control = M3508_M2006_CTRL_ID_EXTAND,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_2,
      },
      Device::RMMotor::Param{
          .id_feedback = 0x205,
          .id_control = M3508_M2006_CTRL_ID_EXTAND,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_2,
      },
    },

    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::Launcher::CHANGE_FIRE_MODE_RELAX
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_TOP,
        Module::Launcher::CHANGE_FIRE_MODE_SAFE
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_MID,
        Module::Launcher::CHANGE_FIRE_MODE_LOADED
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::Launcher::CHANGE_FIRE_MODE_LOADED
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::Launcher::LAUNCHER_START_FIRE
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_L_PRESS,
        Module::Launcher::LAUNCHER_START_FIRE
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_G,
        Module::Launcher::CHANGE_TRIG_MODE
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_R,
        Module::Launcher::OPEN_COVER
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_F,
        Module::Launcher::CLOSE_COVER
      }
    },
  }, /* launcher */

  .bmi088_rot = {
    .rot_mat = {
      { -1, +0, +0},
      { +0, -1, +0},
      { +0, +0, +1},
    },
  },

  .cap = {
    .can = BSP_CAN_1,
    .index = DEV_CAP_FB_ID_BASE,
    .cutoff_volt = 10.0f,
  },
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::Hero, Robot::Hero::Param>(param, 500.0f);
}
