#include "robot.hpp"
/* clang-format off */
Robot::Infantry::Param param = {
    .balance = {
      .init_g_center = 0.108f,

      .follow_pid_param = {
        .k = 0.2f,
        .p = 1.0f,
        .i = 0.5f,
        .d = 0.0f,
        .i_limit = 0.011f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = M_2PI,
      },

      .comp_pid_param = {
        .k = 0.2f,
        .p = 1.0f,
        .i = 0.5f,
        .d = 0.0f,
        .i_limit = 0.011f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = M_2PI,
      },

      .event_map = {
        Component::CMD::CreateMapItem(
          Component::CMD::EventLostCtrl,
          Module::RMBalance::ChangeModeRelax
        ),
        Component::CMD::CreateMapItem(
          Device::DR16::SwitchPosLeftTop,
          Module::RMBalance::ChangeModeRelax
        ),
        Component::CMD::CreateMapItem(
          Device::DR16::SwitchPosLeftMid,
          Module::RMBalance::ChangeModeRelax
        ),
        Component::CMD::CreateMapItem(
          Device::DR16::SwitchPosLeftBot,
          Module::RMBalance::ChangeModeFollow
        )
      },

      .wheel_param = {
        {
          .speed = {
            .k = 0.00025f,
            .p = 1.0f,
            .i = 1.0f,
            .d = 0.0f,
            .i_limit = 1.0f,
            .out_limit = 1.0f,
            .d_cutoff_freq = -1.0f,
            .range = -1.0f,
          },

          .in_cutoff_freq = -1.0f,

          .out_cutoff_freq = -1.0f,
        },{
          .speed = {
            .k = 0.00025f,
            .p = 1.0f,
            .i = 1.0f,
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

      .eulr_param = {
        .k = 2.0f,
        .p = 1.0f,
        .i = 1.0f,
        .d = 0.04f,
        .i_limit = 20.0f,
        .out_limit = 20.0f,
        .d_cutoff_freq = -1.0f,
        .range = M_2PI,
      },

      .gyro_param = {
        .k = 0.15f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = -1.0f,
      },

      .speed_param = {
          .k = 2.0f,
          .p = 1.0f,
          .i = 1.0f,
          .d = 0.0f,
          .i_limit = 0.15f,
          .out_limit = 0.2f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
      },

      .center_filter_cutoff_freq = 10.0f,

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
      },

    },

    .leg = {
      .l1 = 0.11f,
      .l2 = 0.2f,
      .l3 = 0.31f,
      .l4 = 0.455f,

      .limit = {
        .high_max = 0.45,
        .high_min = 0.15,
      },

      .leg_max_angle = 0.0f,

      .motor_zero = {
        0.630508065f,
        5.75973034f,
        1.45487654f,
        3.00471401f,
      },

      .event_map = {
        Component::CMD::CreateMapItem(
          Component::CMD::EventLostCtrl,
          Module::WheelLeg::ChangeModeRelax
        ),
        Component::CMD::CreateMapItem(
          Device::DR16::SwitchPosLeftTop,
          Module::WheelLeg::ChangeModeRelax
        ),
        Component::CMD::CreateMapItem(
          Device::DR16::SwitchPosLeftMid,
          Module::WheelLeg::ChangeModeSquat
        ),
        Component::CMD::CreateMapItem(
          Device::DR16::SwitchPosLeftBot,
          Module::WheelLeg::ChangeModeSquat
        )
      },

      .leg_actr = {
        {
        .speed = {
          .k = 1.0f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 3.0f,
          .out_limit = 3.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .position = {
          .k = 20.0f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .range = M_2PI,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
        },{
        .speed = {
          .k = 1.0f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 3.0f,
          .out_limit = 3.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .position = {
          .k = 20.0f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .range = M_2PI,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
        },{
        .speed = {
          .k = 1.0f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 3.0f,
          .out_limit = 3.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .position = {
          .k = 20.0f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .range = M_2PI,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
        },{
        .speed = {
          .k = 1.0f,
          .p = 1.0f,
          .i = 1.2f,
          .d = 0.0f,
          .i_limit = 3.0f,
          .out_limit = 3.0f,
          .d_cutoff_freq = -1.0f,
          .range = -1.0f,
        },

        .position = {
          .k = 20.0f,
          .p = 1.0f,
          .i = 0.3f,
          .d = 0.0f,
          .i_limit = 0.0f,
          .out_limit = 10.0f,
          .d_cutoff_freq = -1.0f,
          .range = M_2PI,
        },

        .in_cutoff_freq = -1.0f,

        .out_cutoff_freq = -1.0f,
        },
      },

      .leg_motor = {
        {
          .kp = 30.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 1,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
        {
          .kp = 30.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 2,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
        {
          .kp = 30.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 3,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
        {
          .kp = 30.0f,
          .kd = 0.1f,
          .def_speed = 0.0f,
          .id = 4,
          .can = BSP_CAN_1,
          .max_error = 0.1f,
        },
      },
  },

  .bmi088_rot = {
    .rot_mat = {
      { +1, +0, +0},
      { +0, +1, +0},
      { +0, +0, +1},
    },
  },

  .cap = {
    .can = BSP_CAN_1,
    .index = 0x30,
  },

  .can_imu = {
    .tp_name_prefix = "chassis",

    .can = BSP_CAN_1,

    .index = 30,
  }
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
