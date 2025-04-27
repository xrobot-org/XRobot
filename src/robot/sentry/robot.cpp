#include "robot.hpp"

#include <comp_actuator.hpp>

#include "dev_rm_motor.hpp"
#include "mod_chassis.hpp"
#include "system.hpp"

/* clang-format off */
Robot::Sentry::Param param = {
    .chassis={
      .toque_coefficient_ = 0.0327120418848f,
      .speed_2_coefficient_ = 1.227822928729637e-07,
      .out_2_coefficient_ = 1.1108430132455055e-24,
      .constant_ = 1.8135014050213443,
      .type = Component::Mixer::OMNICROSS,

      .follow_pid_param = {
      .k = 1.f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.0f,
      .i_limit = 1.0f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -1.0f,
      .cycle = true,
    },
    .xaccl_pid_param =
      {
      .k = 1.0f,
      .p = 0.6f,
      .i = 1.6f,
      .d = 0.00f,
      .i_limit = 1.0f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -0.001f,
      .cycle = false,
      },
      .yaccl_pid_param =
      {
      .k = 1.0f,
      .p = 0.6f,
      .i = 1.6f,
      .d = 0.00f,
      .i_limit = 1.0f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -0.001f,
      .cycle = false,
      },


    .actuator_param = {
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.00015f,
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
          .k = 0.00018f,
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
          .k = 0.00015f,
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
          .k = 0.00015f,
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
          .id_feedback = 0x204,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse = false
      },
      Device::RMMotor::Param{
          .id_feedback = 0x201,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse = false
      },
      Device::RMMotor::Param{
          .id_feedback = 0x203,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse = false
      },
      Device::RMMotor::Param{
          .id_feedback = 0x202,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse = false
      },
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
        Module::RMChassis::SET_MODE_INDENPENDENT
      },
       Component::CMD::EventMapItem{
        Device::AI::AIControlData::AI_ROTOR,
        Module::RMChassis::SET_MODE_ROTOR
      }
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
          .k = 0.28f,
          .p = 1.f,
          .i = 10.f,
          .d = 0.f,
          .i_limit = 0.2f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_YAW_ANGLE_IDX */
          .k = 15.0f,
          .p = 1.5f,
          .i = 5.f,
          .d = 1.0f,
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
          .k = 0.1f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.f,
          .i_limit = 0.8f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_PIT_ANGLE_IDX */
          .k = 15.0f,
          .p = 1.5f,
          .i = 5.0f,
          .d = 1.0f,
          .i_limit = 0.0f,
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
          .k = 0.28f,
          .p = 1.1f,
          .i = 10.f,
          .d = 0.f,
          .i_limit = 0.2f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_YAW_ANGLE_IDX */
          .k = 25.0f,
          .p = 1.5f,
          .i = 5.f,
          .d = 1.0f,
          .i_limit = 0.0f,
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
          .k = 0.1f,
          .p = 1.2f,
          .i = 0.0f,
          .d = 0.f,
          .i_limit = 0.8f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_PIT_ANGLE_IDX */
          .k = 27.0f,
          .p = 2.2f,
          .i = 6.0f,
          .d = 1.0f,
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
    },
    .yaw_motor = {
      .id_feedback = 0x206,
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
      .yaw = M_2PI-0.46f,
      .pit = M_2PI-2.2,
      .rol = 0.0f,
    },

    .limit = {
      .pitch_max = M_2PI-1.7f,
      .pitch_min = M_2PI-2.4f,
      .yaw_max = 0.0f,
      .yaw_min =  0.0f
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
        Module::Gimbal::SET_MODE_AUTO_AIM
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::Gimbal::SET_MODE_ABSOLUTE
      }
    },

  },

  .launcher = {
    .num_trig_tooth = 8.0f,
    .trig_gear_ratio = 36.0f,
    .fric_radius = 0.03f,
    .cover_open_duty = 0.125f,
    .cover_close_duty = 0.075f,
    .model = Module::RMLauncher::LAUNCHER_MODEL_17MM,
    .default_bullet_speed = 15.f,
    .min_launch_delay = static_cast<uint32_t>(1000.0f / 10.0f),

    .trig_actr = {
      Component::PosActuator::Param{
        .speed = {
          .k = 3.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 1.0f,
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

    .fric_actr = {
     Component::SpeedActuator::Param{
        .speed = {
          .k = 0.00035f,
          .p = 1.0f,
          .i = 0.5f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      Component::SpeedActuator::Param{
        .speed = {
          .k = 0.00035f,
          .p = 1.0f,
          .i = 0.5f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },

    .trig_param = {
      Device::RMMotor::Param{
        .id_feedback = 0x205,
        .id_control = M3508_M2006_CTRL_ID_EXTAND,
        .model = Device::RMMotor::MOTOR_M2006,
        .can = BSP_CAN_1,
        .reverse = false,
      }
    },

    .fric_param = {
      Device::RMMotor::Param{
          .id_feedback = 0x202,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_2,
          .reverse = false,
      },
      Device::RMMotor::Param{
          .id_feedback = 0x203,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_2,
          .reverse = false
      }
    },


    .EVENT_MAP = {
       Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::RMLauncher::CHANGE_FIRE_MODE_RELAX
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_TOP,
        Module::RMLauncher::CHANGE_FIRE_MODE_SAFE
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_MID,
        Module::RMLauncher::CHANGE_FIRE_MODE_SAFE
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::RMLauncher::CHANGE_FIRE_MODE_SAFE
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::RMLauncher::CHANGE_TRIG_MODE_BURST
      },
      Component::CMD::EventMapItem{
        Device::AI::AIControlData::AI_FIRE_COMMAND,
        Module::RMLauncher::CHANGE_TRIG_MODE_BURST
      },
      Component::CMD::EventMapItem{
        Device::AI::AIControlData::AI_STOP_FIRE,
        Module::RMLauncher::LAUNCHER_STOP_TRIG
      }

    },
  }, /* launcher*/
  .bmi088_rot =
            {
                .rot_mat =
                    {
                        {+0, +1, +0},
                        {-1, +0, +0},
                        {+0, +0, +1},
                    },
            },

  .cap = {
    .can = BSP_CAN_1,
  },
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::Sentry, Robot::Sentry::Param>(param, 500.0f);
}
