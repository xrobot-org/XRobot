#include "robot.hpp"

#include <comp_actuator.hpp>

#include "dev_rm_motor.hpp"
// #include "mod_new_omni_chassis.hpp"
#include "system.hpp"

/* clang-format off */
Robot::OmniInfantry::Param param = {
    .chassis={
      .toque_coefficient_ = 0.03985309056810189f,
      .speed_2_coefficient_  = 5.297566618742181e-08f,
      .out_2_coefficient_ = 28.851126052234243f,
      .constant_ = 7.479591458247737f,

      .follow_pid_param = {
      .k = -1.5f,
      //.p = 0.45f,
      .p = 0.70f,
      .i = 0.05f,
      .d = 0.01f,
      .i_limit = 0.5f,
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

    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::RMChassis::SET_MODE_RELAX
      },
      // Component::CMD::EventMapItem{
      //   Device::DR16::DR16_SW_L_POS_TOP,
      //   Module::RMChassis::SET_MODE_RELAX
      // },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_TOP,
        Module::RMChassis::RELAX
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_L_POS_MID,
        Module::RMChassis::SET_MODE_INTERSECT
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
        Module::RMChassis::SET_MODE_INTERSECT
      },
        Component::CMD::EventMapItem{
        Device::DR16::KEY_E,
        Module::RMChassis::SET_MODE_CROSS
      }
    },

    .actuator_param = {
      Component::SpeedActuator::Param{
        .speed = {
          //.k = 0.00050f,
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
          .id_feedback = 0x203,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse=false,
      },
      Device::RMMotor::Param{
          .id_feedback = 0x204,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse=false,
      },
      Device::RMMotor::Param{
          .id_feedback = 0x201,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse=false,
      },
      Device::RMMotor::Param{
          .id_feedback = 0x202,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
          .reverse=false,
      },
    },
      .get_speed = [](float power_limit){
      float speed = 0.0f;
      if (power_limit <= 50.0f) {
        speed = 5500;
      } else if (power_limit <= 60.0f) {
        speed = 6000;
      } else if (power_limit <= 70.0f) {
        speed = 6500;
      } else if (power_limit <= 80.0f) {
        speed = 7000;
      } else if (power_limit <= 100.0f) {
        speed = 7500;
      } else {
        speed = 8000;
      }
      return speed;
    },
  },

  .gimbal= {
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
          .k = 0.4f,
          .p = 1.f,
          .i = 0.5f,
          .d = 0.f,
          .i_limit = 0.2f,
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
          /* GIMBAL_CTRL_YAW_OMEGA_IDX */
          .k = 0.4f,
          .p = 0.8f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.2f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_YAW_ANGLE_IDX */
          .k = 20.0f,
          .p = 0.75f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.2f,
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
          .k = 0.4f,
          .p = 1.f,
          .i = 0.5f,
          .d = 0.f,
          .i_limit = 0.2f,
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
    .pit_ai_actr = {
      .speed = {
          /* GIMBAL_CTRL_YAW_OMEGA_IDX */
          .k = 0.4f,
          .p = 0.8f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.2f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          /* GIMBAL_CTRL_YAW_ANGLE_IDX */
          .k = 20.0f,
          .p = 0.75f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.2f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
    },
    .yaw_motor = {
      .id_feedback = 0x205,
      .id_control = GM6020_CTRL_ID_BASE,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_1,
      .reverse = true,
    },
    .pit_motor ={
      .id_feedback = 0x207,
      .id_control = GM6020_CTRL_ID_BASE,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_2,
      .reverse = true,
    },
    .mech_zero = {
      .yaw = M_2PI - 3.69229198f,
      .pit = 5.82529211,
      .rol = 0.0f,
    },

    .limit = {
      .pitch_max = M_2PI - 5.49569994f,
      .pitch_min = M_2PI - 6.15663195f,
      .yaw_max = 0.0f,
      .yaw_min = 0.0f,
    },

    .EVENT_MAP = {
      Component::CMD::EventMapItem{
        Component::CMD::CMD_EVENT_LOST_CTRL,
        Module::Gimbal::SET_MODE_RELAX
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_TOP,
        Module::Gimbal::ABSOLUTE
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
        Module::Gimbal::SET_MODE_AUTO_AIM
      },
      // Component::CMD::EventMapItem{
      //   Device::DR16::DR16_SW_R_POS_MID,
      //   Module::Gimbal::ABSOLUTE
      // },
      // Component::CMD::EventMapItem{
      //   Device::DR16::DR16_SW_R_POS_BOT,
      //   Module::Gimbal::ABSOLUTE
      // },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_R_PRESS,
        Module::Gimbal::SET_MODE_AUTO_AIM
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_R_RELEASE,
        Module::Gimbal::SET_MODE_ABSOLUTE
      }
    },
  },

  .launcher = {
    .num_trig_tooth = 10.0f,
    .trig_gear_ratio = 36.0f * 2.5f,
    .fric_radius = 0.03f,
    .cover_open_duty = 0.125f,
    .cover_close_duty = 0.075f,
    .model = Module::RMLauncher::LAUNCHER_MODEL_17MM,
    .default_bullet_speed = 25.0f,
    .min_launch_delay = static_cast<uint32_t>(1000.0f / 12.0f),

    .trig_actr = {
      Component::PosActuator::Param{
        .speed = {
          .k = 3.0f,
          .p = 2.0f,
          .i = 0.5f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 1.8f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.0f,
          .out_limit = 0.55f,
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
            .k = 0.00025f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 0.2f,
            .out_limit = 1.0f,
            .d_cutoff_freq = -1.0f,
            .cycle = false,
            },

            .in_cutoff_freq = -1.0f,

            .out_cutoff_freq = -1.0f,
            },
           Component::SpeedActuator::Param{
            .speed = {
            .k = 0.00025f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 0.2f,
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
        .id_feedback = 0x207,
        .id_control = M3508_M2006_CTRL_ID_EXTAND,
        .model = Device::RMMotor::MOTOR_M2006,
        .can = BSP_CAN_1,
        .reverse = true,
      }
    },

    .fric_param = {
      Device::RMMotor::Param{
          .id_feedback = 0x201,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_2,
          .reverse=false,
      },
      Device::RMMotor::Param{
          .id_feedback = 0x203,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_2,
          .reverse=false,
      },
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
        Module::RMLauncher::CHANGE_FIRE_MODE_LOADED
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_MID,
        Module::RMLauncher::LAUNCHER_STOP_TRIG
      },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::RMLauncher::CHANGE_FIRE_MODE_LOADED
      },
      // Component::CMD::EventMapItem{
      //   Device::DR16::DR16_SW_R_POS_BOT,
      //   Module::RMLauncher::LAUNCHER_START_FIRE
      // },
      Component::CMD::EventMapItem{
        Device::DR16::DR16_SW_R_POS_BOT,
        Module::RMLauncher::CHANGE_TRIG_MODE_BURST
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_L_PRESS,
        Module::RMLauncher::CHANGE_TRIG_MODE_BURST
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_L_RELEASE,
        Module::RMLauncher::LAUNCHER_STOP_TRIG
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_G,
        Module::RMLauncher::CHANGE_TRIG_MODE
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_R,
        Module::RMLauncher::OPEN_COVER
      },
      Component::CMD::EventMapItem{
        Device::DR16::KEY_F,
        Module::RMLauncher::CLOSE_COVER
      },
      // Component::CMD::EventMapItem{
      //   Device::DR16::KEY_V,
      // }
      //   Module::RMLauncher::CHANGE_AI_MODE
    },
  }, /* launcher */

  .bmi088_rot = {
    .rot_mat = {
      { +0, +1, +0},
      { -1, +0, +0},
      { +0, +0, +1},
    },
  },

  .cap = {
    .can = BSP_CAN_1,
  },
};
/* clang-format on */

void robot_init() {
  System::Start<Robot::OmniInfantry, Robot::OmniInfantry::Param>(param, 500.0f);
}
