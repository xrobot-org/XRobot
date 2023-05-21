#include "robot.hpp"

#include "system.hpp"

/* clang-format off */
Robot::Infantry::Param param = {
    .balance = {
      .init_g_center = 0.11f,

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
          Module::RMBalance::SET_MODE_FOLLOW
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_BOT,
          Module::RMBalance::SET_MODE_ROTOR
        },
        Component::CMD::EventMapItem{
          Device::DR16::KEY_V,
          Module::RMBalance::SET_MODE_ROTOR
        },
        Component::CMD::EventMapItem{
          Device::DR16::KEY_B,
          Module::RMBalance::SET_MODE_FOLLOW
        }
      },

      .speed_filter_cutoff_freq = 0.0f,

      .motor_param = {
        Device::RMDMotor::Param{
          .num = 1,
          .can = BSP_CAN_2,
          .reverse = true,
        },
        Device::RMDMotor::Param{
          .num = 0,
          .can = BSP_CAN_2,
          .reverse = true,
        },
      },

      .pid_param = {
        /* CTRL_CH_DISPLACEMENT */
        Component::PID::Param{
          .k = 1.3f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 8.0f,
          .i_limit = 0.0f,
          .out_limit = 0.15f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        /* CTRL_CH_FORWARD_SPEED */
        Component::PID::Param{
          .k = 0.4f,
          .p = 1.0f,
          .i = 1.0f,
          .d = 0.0f,
          .i_limit = 0.1f,
          .out_limit = 0.1f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        /* CTRL_CH_PITCH_ANGLE */
        Component::PID::Param{
          .k = 1.5f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.1f,
          .out_limit = 0.25f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },
        /* CTRL_CH_GYRO_X */
        Component::PID::Param{
          .k = 0.15f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.1f,
          .out_limit = 0.2f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
        /* CTRL_CH_YAW_ANGLE */
        Component::PID::Param{
          .k = 0.2f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.1f,
          .out_limit = 0.14f,
          .d_cutoff_freq = -1.0f,
          .cycle = true,
        },
        /* CTRL_CH_GYRO_Z */
        Component::PID::Param{
          .k = 8.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.1f,
          .out_limit = 0.1f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },
      },

      .offset_pid = {
        .k = 1.0f,
        .p = 1.0f,
        .i = 1.0f,
        .d = 0.0f,
        .i_limit = 0.05f,
        .out_limit = 0.5f,
        .d_cutoff_freq = -1.0f,
        .cycle = true,
      },
    },

    .leg = {
      .l1 = 0.11f,
      .l2 = 0.15f,
      .l3 = 0.25f,
      .l4 = 0.54f,

      .limit = {
        .high_max = 0.45f,
        .high_min = 0.14f,
      },

      .leg_max_angle = 0.0f,

      .motor_zero = {
        5.21f,
        4.22f,
        1.79f,
        3.62f,
      },


      .EVENT_MAP = {
        Component::CMD::EventMapItem{
          Component::CMD::CMD_EVENT_LOST_CTRL,
          Module::WheelLeg::SET_MODE_RELAX
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_TOP,
          Module::WheelLeg::SET_MODE_BREAK
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_MID,
          Module::WheelLeg::SET_MODE_BREAK
        },
        Component::CMD::EventMapItem{
          Device::DR16::DR16_SW_L_POS_BOT,
          Module::WheelLeg::SET_MODE_BREAK
        }
      },

      .leg_actr = {
        Component::PosActuator::Param{
        .speed = {
          .k = 0.5f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 2.0f,
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
          .k = 0.5f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 2.0f,
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
          .k = 0.5f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 2.0f,
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
          .k = 0.5f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 2.0f,
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
          .kp = 50.0f,
          .kd = 0.05f,
          .def_speed = 0.0f,
          .id = 1,
          .can = BSP_CAN_2,
          .max_error = 0.1f,
        },
        Device::MitMotor::Param{
          .kp = 50.0f,
          .kd = 0.05f,
          .def_speed = 0.0f,
          .id = 2,
          .can = BSP_CAN_2,
          .max_error = 0.1f,
        },
        Device::MitMotor::Param{
          .kp = 50.0f,
          .kd = 0.05f,
          .def_speed = 0.0f,
          .id = 1,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
        Device::MitMotor::Param{
          .kp = 50.0f,
          .kd = 0.05f,
          .def_speed = 0.0f,
          .id = 4,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
      },
  },

    .launcher = {
    .num_trig_tooth = 8.0f,
    .trig_gear_ratio = 36.0f,
    .fric_radius = 0.03f,
    .cover_open_duty = 0.0850000009,
    .cover_close_duty = 0.0329999998f,
    .model = Module::Launcher::LAUNCHER_MODEL_17MM,
    .default_bullet_speed = 30.f,
    .min_launch_delay = static_cast<uint32_t>(1000.0f / 20.0f),

    .trig_actr = {
      Component::PosActuator::Param{
        .speed = {
          .k = 2.0f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .cycle = false,
        },

        .position = {
          .k = 0.4f,
          .p = 1.0f,
          .i = 0.5f,
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
          .k = 0.0002f,
          .p = 1.0f,
          .i = 0.4f,
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
          .k = 0.0002f,
          .p = 1.0f,
          .i = 0.4f,
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

    .trig_motor = {
      Device::RMMotor::Param{
        .id_feedback = 0x202,
        .id_control = M3508_M2006_CTRL_ID_BASE,
        .model = Device::RMMotor::MOTOR_M2006,
        .can = BSP_CAN_4,
      }
    },

    .fric_motor = {
      Device::RMMotor::Param{
          .id_feedback = 0x203,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_4,
      },
      Device::RMMotor::Param{
          .id_feedback = 0x204,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_4,
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
          .k = 0.4f,
          .p = 1.0f,
          .i = 0.1f,
          .d = 0.0f,
          .i_limit = 0.4f,
          .out_limit = 0.8f,
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
          .k = 0.2f,
          .p = 1.0f,
          .i = 0.2f,
          .d = 0.0f,
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
      .can = BSP_CAN_1,
    },

    .pit_motor = {
      .id_feedback = 0x206,
      .id_control = GM6020_CTRL_ID_BASE,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_4,
    },

    .mech_zero = {
      .yaw = 0.96f,
      .pit = 4.0f,
      .rol = 0.0f,
    },

    .limit = {
      .pitch_max = 5.9f,
      .pitch_min = 4.89f,
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


  .bmi088_rot = {
    .rot_mat = {
      {+1, +0, +0},
      {+0, +1, +0},
      {+0, +0, +1},
    },
  },

  .cap = {
    .can = BSP_CAN_2,
    .index = DEV_CAP_FB_ID_BASE,
    .cutoff_volt = 13.0f,
  },

  .can_imu = {
    .tp_name_prefix = "chassis",

    .can = BSP_CAN_1,

    .index = 50,
  },

  .blink = {
    .gpio = BSP_GPIO_LED,
    .timeout = 200,
  }
};
/* clang-format on */

Robot::Infantry* Robot::Infantry::self_;

void robot_init() {
  System::Start<Robot::Infantry, Robot::Infantry::Param>(param, 500.0f);
}
