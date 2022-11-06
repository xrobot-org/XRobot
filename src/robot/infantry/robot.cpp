#include "robot.hpp"
/* clang-format off */
Robot::Infantry::Param param = {
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
      .range = M_2PI,
    },

    .event_map = {
      Component::CMD::CreateMapItem(
        Component::CMD::EventLostCtrl,
        Module::RMChassis::ChangeModeRelax
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosLeftTop,
        Module::RMChassis::ChangeModeRelax
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosLeftMid,
        Module::RMChassis::ChangeModeFollow
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosLeftBot,
        Module::RMChassis::ChangeModeRotor
      )
    },

    .actuator_param = {
      {
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,

      },
      {
        .speed = {
          .k = 0.00018f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      {
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      {
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.0f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },

    .motor_param = {
      {
          .id_feedback = 0x201,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
      },
      {
          .id_feedback = 0x202,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
      },
      {
          .id_feedback = 0x203,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_1,
      },
      {
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
          .k = 0.14f,
          .p = 1.f,
          .i = 3.f,
          .d = 0.f,
          .i_limit = 0.2f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
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
          .range = M_2PI,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
    },
    .pit_actr = {
        .speed = {
          /* GIMBAL_CTRL_PIT_OMEGA_IDX */
          .k = 0.1f,
          .p = 1.0f,
          .i = 0.f,
          .d = 0.f,
          .i_limit = 0.8f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
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
          .range = M_2PI,
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
      .id_feedback = 0x20A,
      .id_control = GM6020_CTRL_ID_EXTAND,
      .model = Device::RMMotor::MOTOR_GM6020,
      .can = BSP_CAN_2,
    },

    .mech_zero = {
      .yaw = 1.3f,
      .pit = 4.0f,
      .rol = 0,
    },

    .limit = {
      .pitch_max = 3.8f,
      .pitch_min = 3.0f,
    },

    .event_map = {
      Component::CMD::CreateMapItem(
        Component::CMD::EventLostCtrl,
        Module::Gimbal::SetModeRelax
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosRightTop,
        Module::Gimbal::SetModeAbsolute
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosRightMid,
        Module::Gimbal::SetModeAbsolute
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosRightBot,
        Module::Gimbal::SetModeAbsolute
      )
    },

  },

  .launcher = {
    .num_trig_tooth = 8.0f,
    .trig_gear_ratio = 36.0f,
    .fric_radius = 0.03f,
    .cover_open_duty = 0.125f,
    .cover_close_duty = 0.075f,
    .model = Module::Launcher::LAUNCHER_MODEL_17MM,
    .default_bullet_speed = 30.f,
    .min_launch_delay = (uint32_t)(1000.0f / 20.0f),

    .trig_actr = {
      {
        .speed = {
          .k = 1.5f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.03f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .position = {
          .k = 1.2f,
          .p = 1.0f,
          .i = 0.0f,
          .d = 0.012f,
          .i_limit = 1.0f,
          .out_limit = 1.0f,
          .d_cutoff_freq = -1.0f,
          .range = M_2PI,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },

    .fric_actr = {
      {
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.4f,
          .d = 0.01f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
      {
        .speed = {
          .k = 0.00015f,
          .p = 1.0f,
          .i = 0.4f,
          .d = 0.01f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
      },
    },

    .trig_motor = {
      {
        .id_feedback = 0x207,
        .id_control = M3508_M2006_CTRL_ID_EXTAND,
        .model = Device::RMMotor::MOTOR_M2006,
        .can = BSP_CAN_2,
      }
    },

    .fric_motor = {
      {
          .id_feedback = 0x205,
          .id_control = M3508_M2006_CTRL_ID_EXTAND,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_2,
      },
      {
          .id_feedback = 0x206,
          .id_control = M3508_M2006_CTRL_ID_EXTAND,
          .model = Device::RMMotor::MOTOR_M3508,
          .can = BSP_CAN_2,
      },
    },

    .event_map = {
      Component::CMD::CreateMapItem(
        Component::CMD::EventLostCtrl,
        Module::Launcher::ChangeFireModeRelax
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosRightTop,
        Module::Launcher::ChangeFireModeSafe
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosRightMid,
        Module::Launcher::ChangeFireModeLoaded
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosRightBot,
        Module::Launcher::ChangeFireModeLoaded
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::SwitchPosRightBot,
        Module::Launcher::StartFire
      ),
      Component::CMD::CreateMapItem(
        Device::DR16::KeyLClick,
        Module::Launcher::StartFire
      )
    },
  }, /* launcher */

  .bmi088_rot = {
    .rot_mat = {
      { +1, +0, +0},
      { +0, +1, +0},
      { +0, +0, +1},
    },
  },

  .cap = {
    .can = BSP_CAN_1,
    .index = DEV_CAP_FB_ID_BASE,
  },
};
/* clang-format on */

void robot_init() {
  auto init_thread_fn = [](void* arg) {
    RM_UNUSED(arg);

    Robot::Infantry robot(param, 500.0f);
    while (1) {
      System::Thread::Sleep(UINT32_MAX);
    }
  };

  System::Thread init_thread;
  init_thread.Create(init_thread_fn, (void*)0, "init_thread_fn", 1024,
                     System::Thread::Realtime);
}
