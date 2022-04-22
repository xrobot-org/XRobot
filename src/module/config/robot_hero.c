#include "mod_config.h"

#if ID_HERO

/* clang-format off */
#ifdef MCU_DEBUG_BUILD
config_robot_param_t param_robot = {
#else
const config_robot_param_t param_robot = {
#endif
  .name = "hero",

  .chassis = { /* 底盘模块参数 */
    .type = CHASSIS_TYPE_MECANUM,

    .motor_pid_param = {
      .k = 0.0003f,
      .p = 1.0f,
      .i = 0.001f,
      .d = 0.0f,
      .i_limit = 1.0f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -1.0f,
      .range = -1.0f,
    },

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

    .low_pass_cutoff_freq = {
      .in = -1.0f,
      .out = -1.0f,
    },

    .reverse = {
      .yaw = true,
    },
  }, /* chassis */

  .gimbal = { /* 云台模块参数 */
    .type = GIMBAL_TYPE_ROTATRY,

    .ff = {
        /* GIMBAL_CTRL_PIT_FEEDFORWARD */
        .a = -0.5481f,
        .b = -0.0996f,
        .c = -0.0337f,
        .max = 0.1f,
        .min = -0.5f,
    }, /* ff */

    .st = {
        /* GIMBAL_CTRL_YAW_SELF_TUNING */
        .a = -0.3316f,
        .b = 0.0653f,
        .c = 0.8994f,
        .max = 0.92f,
        .min = 0.75f,
    }, /* st */

    .pid = {
      {
        /* GIMBAL_CTRL_YAW_OMEGA_IDX */
        .k = 0.05f,
        .p = 1.0f,
        .i = 6.0f,
        .d = 0.0008f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = -1.0f,
      }, {
        /* GIMBAL_CTRL_YAW_ANGLE_IDX */
        .k = 20.0f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 0.0f,
        .out_limit = 10.0f,
        .d_cutoff_freq = -1.0f,
        .range = M_2PI,
      }, {
        /* GIMBAL_CTRL_PIT_OMEGA_IDX */
        .k = 0.3f,
        .p = 1.0f,
        .i = 1.5f,
        .d = 0.0f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = -1.0f,
      }, {
        /* GIMBAL_CTRL_PIT_ANGLE_IDX */
        .k = 12.0f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 0.0f,
        .out_limit = 10.0f,
        .d_cutoff_freq = -1.0f,
        .range = M_2PI,
      },
    }, /* pid */

    .pitch_travel_rad = 1.07685447f,

    .low_pass_cutoff_freq = {
      .out = -1.0f,
      .gyro = 1000.0f,
    },

    .reverse = {
      .yaw = true,
      .pit = true,
    },
  }, /* gimbal */

  .launcher = { /* 发射器模块参数 */

    .pid_param[LAUNCHER_CTRL_FRIC1_SPEED_IDX] = {
      .k = 0.00015f,
      .p = 1.0f,
      .i = 0.4f,
      .d = 0.01f,
      .i_limit = 0.5f,
      .out_limit = 0.5f,
      .d_cutoff_freq = -1.0f,
    },

    .pid_param[LAUNCHER_CTRL_FRIC2_SPEED_IDX] = {
      .k = 0.00015f,
      .p = 1.0f,
      .i = 0.4f,
      .d = 0.01f,
      .i_limit = 0.5f,
      .out_limit = 0.5f,
      .d_cutoff_freq = -1.0f,
    },

    .pid_param[LAUNCHER_CTRL_TRIG_SPEED_IDX] = {
      .k = 0.0015f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.03f,
      .i_limit = 0.5f,
      .out_limit = 0.5f,
      .d_cutoff_freq = -1.0f,
    },

    .pid_param[LAUNCHER_CTRL_TRIG_ANGLE_IDX] = {
      .k = 0.6f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.012f,
      .i_limit = 1.0f,
      .out_limit = 1.0f,
      .d_cutoff_freq = -1.0f,
      .range = M_2PI,
    },

    .low_pass_cutoff_freq = {
      .in = {
        .fric = -1.0f,
        .trig = -1.0f,
      },
      .out = {
        .fric = -1.0f,
        .trig = -1.0f,
      },
    },

    .num_trig_tooth = 6.0f,
    .trig_gear_ratio = 3591.0f / 187.0f,
    .fric_radius = 0.03f,
    .cover_open_duty = 0.125f,
    .cover_close_duty = 0.075f,
    .model = LAUNCHER_MODEL_42MM,
    .default_bullet_speed = 16.0f,
    .min_launch_delay = (uint32_t)(1000.0f / 20.0f),
  }, /* launcher */

  .imu = {
    .rot_mat = {
      { -1, +0, +0},
      { +0, -1, +0},
      { +0, +0, +1},
    },
  }, /* imu */

  .motor = {
    [MOTOR_GROUP_ID_CHASSIS] = {
      .id_feedback = 0x201,
      .id_control = M3508_M2006_CTRL_ID_BASE,
      .model = {MOTOR_M3508, MOTOR_M3508, MOTOR_M3508, MOTOR_M3508},
      .num = 4,
      .can = BSP_CAN_1,
    },
    [MOTOR_GROUP_ID_LAUNCHER_FRIC] = {
      .id_feedback = 0x205,
      .id_control = M3508_M2006_CTRL_ID_EXTAND,
      .model = {MOTOR_M3508, MOTOR_M3508, MOTOR_NONE, MOTOR_NONE},
      .num = 2,
      .can = BSP_CAN_2,
    },
    [MOTOR_GROUP_ID_LAUNCHER_TRIG] = {
      .id_feedback = 0x207,
      .id_control = M3508_M2006_CTRL_ID_EXTAND,
      .model = {MOTOR_M2006, MOTOR_NONE, MOTOR_NONE, MOTOR_NONE},
      .num = 1,
      .can = BSP_CAN_1,
    },
    [MOTOR_GROUP_ID_GIMBAL_YAW] = {
      .id_feedback = 0x209,
      .id_control = GM6020_CTRL_ID_EXTAND,
      .model = {MOTOR_GM6020, MOTOR_NONE, MOTOR_NONE, MOTOR_NONE},
      .num = 1,
      .can = BSP_CAN_2,
    },
    [MOTOR_GROUP_ID_GIMBAL_PIT] = {
      .id_feedback = 0x20A,
      .id_control = GM6020_CTRL_ID_EXTAND,
      .model = {MOTOR_GM6020, MOTOR_NONE, MOTOR_NONE, MOTOR_NONE},
      .num = 1,
      .can = BSP_CAN_2,
    },
  },

  .cap = {
    .can = BSP_CAN_1,
    .index = DEV_CAP_FB_ID_BASE,
    .num = DEV_CAP_NUMBER,
  },

  .tof = {
    .can = BSP_CAN_1,
    .index = DEV_TOF_ID_BASE,
    .num = DEV_TOF_SENSOR_NUMBER,
  },

  .default_mode = {
    .chassis = CHASSIS_MODE_FOLLOW_GIMBAL,
    .gimbal = GIMBAL_MODE_ABSOLUTE,
    .launcher = LAUNCHER_MODE_LOADED,
  },
}; /* param_hero */

/* clang-format on */

#endif
