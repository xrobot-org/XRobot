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

        .motor = {
          .id_feedback = 0x201,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::Motor::MOTOR_M3508,
          .num = 0,
          .can = BSP_CAN_1,
        },

        .reverse = false,
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

        .motor = {
          .id_feedback = 0x202,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::Motor::MOTOR_M3508,
          .num = 1,
          .can = BSP_CAN_1,
        },

        .reverse = false,
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

        .motor = {
          .id_feedback = 0x203,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::Motor::MOTOR_M3508,
          .num = 2,
          .can = BSP_CAN_1,
        },

        .reverse = false,
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

        .motor = {
          .id_feedback = 0x204,
          .id_control = M3508_M2006_CTRL_ID_BASE,
          .model = Device::Motor::MOTOR_M3508,
          .num = 3,
          .can = BSP_CAN_1,
        },

        .reverse = false,
      },
    },
    .limit_param = {
      .min_k_percent = 0.6f,
      .max_k_percent = 4.0f,
      .max_load = 10.0f,
      .min_current = 0.1f,
      .filter_k = 0.05f,
    }
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

    .actuator = {
      {
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

        .pos = {
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

        .motor = {
          .id_feedback = 0x209,
          .id_control = GM6020_CTRL_ID_EXTAND,
          .model = Device::Motor::MOTOR_GM6020,
          .num = 0,
          .can = BSP_CAN_1,
        },

        .reverse = false,
      },
      {
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

        .pos = {
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

        .motor = {
          .id_feedback = 0x20A,
          .id_control = GM6020_CTRL_ID_EXTAND,
          .model = Device::Motor::MOTOR_GM6020,
          .num = 1,
          .can = BSP_CAN_2,
        },

        .reverse = false,
      },
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

    .trig_motor = {
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

        .pos = {
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

        .motor = {
          .id_feedback = 0x207,
          .id_control = M3508_M2006_CTRL_ID_EXTAND,
          .model = Device::Motor::MOTOR_M2006,
          .num = 2,
          .can = BSP_CAN_2,
        },

        .reverse = false,
      },
    },

    .fric_motor = {
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

        .motor = {
          .id_feedback = 0x205,
          .id_control = M3508_M2006_CTRL_ID_EXTAND,
          .model = Device::Motor::MOTOR_M3508,
          .num = 0,
          .can = BSP_CAN_2,
        },

        .reverse = false,
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

        .motor = {
          .id_feedback = 0x206,
          .id_control = M3508_M2006_CTRL_ID_EXTAND,
          .model = Device::Motor::MOTOR_M3508,
          .num = 1,
          .can = BSP_CAN_2,
        },

        .reverse = false,
      },
    }
  }, /* launcher */

  .bmi088_rot = {
    .rot_mat = {
      { +1, +0, +0},
      { +0, +1, +0},
      { +0, +0, +1},
    },
  },

  .bmi088_cali = {
    .gyro_offset = {
      .x = 0.000332771f,
      .y = 0.004123951f,
      .z = -0.000634991f,
    },
  },

  .cap = {
    .can = BSP_CAN_1,
    .index = DEV_CAP_FB_ID_BASE,
  },

  .cmd = {
    .sens_mouse = 0.06f,
    .sens_stick = 6.0f,
    .key_map = {
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_W},        /* 向前 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_S},        /* 向后 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_A},        /* 向左 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_D},        /* 向右 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_SHIFT},    /* 加速 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_CTRL},     /* 减速 */
            {Component::CMD::CMD_ACTIVE_PRESSED, Component::CMD::CMD_KEY_L_CLICK},  /* 开火 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_R_CLICK}, /* 切换开火模式 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_E},       /* 自瞄模式 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_F},       /* 弹舱盖开关 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_R},       /* 小陀螺模式 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_G},       /* 反转拨弹 */
            {Component::CMD::CMD_ACTIVE_PRESSING, Component::CMD::CMD_KEY_C}, },    /* 跟随云台呈35度 */

    .move = {
      .sense_norm = 0.8f,
      .sense_fast = 1.25f,
      .sense_slow = 0.8f,
    },

    .default_mode = {
      .gimbal = Component::CMD::GIMBAL_MODE_ABSOLUTE,
      .chassis = Component::CMD::CHASSIS_MODE_FOLLOW_GIMBAL,
      .launcher = Component::CMD::LAUNCHER_MODE_LOADED,
    }
  },



};
/* clang-format on */

void robot_init() {
  auto init_thread_fn = [](void* arg) {
    RM_UNUSED(arg);

    Robot::Infantry infantry(param);
    while (1) {
      System::Thread::Sleep(UINT32_MAX);
    }
  };

  System::Thread init_thread;
  THREAD_DECLEAR(init_thread, init_thread_fn, 1024, System::Thread::Realtime,
                 NULL);
}
