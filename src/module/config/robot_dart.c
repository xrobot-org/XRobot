#include "mod_config.h"

#if ID_DART

/* clang-format off */
#ifdef MCU_DEBUG_BUILD
config_robot_param_t param_robot = {
#else
const config_robot_param_t param_robot = {
#endif
  .name = "dart",

  .chassis = { /* 底盘模块参数 */
    .type = CHASSIS_TYPE_NONE,
  }, /* chassis */

  .gimbal = { /* 云台模块参数 */
    .type = GIMBAL_TYPE_LINEAR,

    .pid = {
      {
        /* GIMBAL_CTRL_YAW_OMEGA_IDX */
        .k = 0.3f,
        .p = 1.0f,
        .i = 0.5f,
        .d = 0.002f,
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
        .k = 0.1,
        .p = 1.0f,
        .i = 1.0f,
        .d = 0.0f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = -1.0f,
      }, {
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
    }, /* pid */

    .pitch_travel_rad = 1.45f,

    .low_pass_cutoff_freq = {
      .out = -1.0f,
      .gyro = 1000.0f,
    },

    .reverse = {
      .yaw = false,
      .pit = false,
    },
  }, /* gimbal */

  .launcher = { /* 发射器模块参数 */

    .fric_pid_param = {
      .k = 0.00015f,
      .p = 1.0f,
      .i = 0.4f,
      .d = 0.01f,
      .i_limit = 0.5f,
      .out_limit = 0.5f,
      .d_cutoff_freq = -1.0f,
    },

    .trig_pid_param = {
      .k = 4.5f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.032f,
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

    .num_trig_tooth = 8.0f,
    .trig_gear_ratio = 36.0f,
    .fric_radius = 0.03f,
    .cover_open_duty = 0.125f,
    .cover_close_duty = 0.075f,
    .model = LAUNCHER_MODEL_17MM,
    .default_bullet_speed = 5.0f,
    .min_launch_delay = (uint32_t)(1000.0f / 20.0f),
  }, /* launcher */

  .imu = {
    .rot_mat = {
      { +0, +1, +0},
      { +1, +0, +0},
      { +0, +0, -1},
    },
  }, /* imu */

  .motor = {
    [MOTOR_GROUP_ID_CHASSIS] = {
      .id_feedback = 0x201,
      .id_control = M3508_M2006_CTRL_ID_BASE,
      .model = {MOTOR_NONE, MOTOR_NONE, MOTOR_NONE, MOTOR_NONE},
      .num = 1,
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
      .id_feedback = 0x202,
      .id_control = M3508_M2006_CTRL_ID_BASE,
      .model = {MOTOR_M2006, MOTOR_NONE, MOTOR_NONE, MOTOR_NONE},
      .num = 1,
      .can = BSP_CAN_1,
    },
    [MOTOR_GROUP_ID_GIMBAL_YAW] = {
      .id_feedback = 0x209,
      .id_control = GM6020_CTRL_ID_EXTAND,
      .model = {MOTOR_GM6020, MOTOR_NONE, MOTOR_NONE, MOTOR_NONE},
      .num = 1,
      .can = BSP_CAN_1,
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
    .chassis = CHASSIS_MODE_SCAN,
    .gimbal = GIMBAL_MODE_SCAN,
    .launcher = LAUNCHER_MODE_LOADED,
  },
}; /* param_sentry */

/* clang-format on */

#endif
