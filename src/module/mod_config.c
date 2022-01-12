/*
 * 配置相关
 */

#include "mod_config.h"

#include <stdint.h>
#include <string.h>

#include "bsp_flash.h"

#define CONFIG_BASE_ADDRESS (ADDR_FLASH_SECTOR_11)

/* clang-format off */
#ifdef MCU_DEBUG_BUILD
config_robot_param_t param_default = {
#else
static const config_robot_param_t param_default = {
#endif
  .model = ROBOT_MODEL_INFANTRY,

  .chassis = { /* 底盘模块参数 */
    .type = CHASSIS_TYPE_MECANUM,

    .motor_pid_param = {
      .k = 0.001f,
      .p = 1.0f,
      .i = 0.0f,
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
      .yaw = false,
    },

  }, /* chassis */

  .gimbal = { /* 云台模块参数 */
    .pid = {
      {
        /* GIMBAL_CTRL_YAW_OMEGA_IDX */
        .k = 0.24f,
        .p = 1.f,
        .i = 3.f,
        .d = 0.f,
        .i_limit = 0.8f,
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
        .k = 0.2f,
        .p = 1.0f,
        .i = 0.f,
        .d = 0.f,
        .i_limit = 0.8f,
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

    .pitch_travel_rad = 1.158155117179586476925286766559f,

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
      .k = 0.001f,
      .p = 1.0f,
      .i = 0.2f,
      .d = 0.01f,
      .i_limit = 0.5f,
      .out_limit = 0.5f,
      .d_cutoff_freq = -1.0f,
    },
    .trig_pid_param = {
      .k = 8.0f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.0450000018f,
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
    .default_bullet_speed = 30.f,
    .min_launch_delay = (uint32_t)(1000.0f / 20.0f),
  }, /* launcher */

  .motor = {
    [MOTOR_GROUT_ID_CHASSIS] = {
      .id_feedback = 0x201,
      .id_control = M3508_M2006_CTRL_ID_BASE,
      .model = {MOTOR_M3508, MOTOR_M3508, MOTOR_M3508, MOTOR_M3508},
      .num = 4,
    },
    [MOTOR_GROUT_ID_GIMBAL] = {
      .id_feedback = 0x205,
      .id_control = GM6020_CTRL_ID_EXTAND,
      .model = {MOTOR_M3508, MOTOR_M3508, MOTOR_M2006, MOTOR_NONE},
      .num = 2,
    },
    [MOTOR_GROUT_ID_LAUNCHER] = {
      .id_feedback = 0x209,
      .id_control = M3508_M2006_CTRL_ID_EXTAND,
      .model = {MOTOR_GM6020, MOTOR_GM6020, MOTOR_NONE, MOTOR_NONE},
      .num = 3,
    },
  },
}; /* param_default */

static const config_robot_param_t param_hero = {
  .model = ROBOT_MODEL_HERO,

  .chassis = { /* 底盘模块参数 */
    .type = CHASSIS_TYPE_MECANUM,

    .motor_pid_param = {
      .k = 0.0011f,
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
    .pid = {
      {
        /* GIMBAL_CTRL_YAW_OMEGA_IDX */
        .k = 0.45f,
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
        .k = 0.25f,
        .p = 1.0f,
        .i = 0.0f,
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

    .fric_pid_param = {
      .k = 0.001f,
      .p = 1.0f,
      .i = 0.2f,
      .d = 0.01f,
      .i_limit = 0.5f,
      .out_limit = 0.5f,
      .d_cutoff_freq = -1.0f,
    },

    .trig_pid_param = {
      .k = 10.0f,
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

    .num_trig_tooth = 6.0f,
    .trig_gear_ratio = 3591.0f / 187.0f,
    .fric_radius = 0.03f,
    .cover_open_duty = 0.125f,
    .cover_close_duty = 0.075f,
    .model = LAUNCHER_MODEL_42MM,
    .default_bullet_speed = 16.0f,
    .min_launch_delay = (uint32_t)(1000.0f / 20.0f),
  }, /* launcher */

  .motor = {
    [MOTOR_GROUT_ID_CHASSIS] = {
      .id_feedback = 0x201,
      .id_control = M3508_M2006_CTRL_ID_BASE,
      .model = {MOTOR_M3508, MOTOR_M3508, MOTOR_M3508, MOTOR_M3508},
      .num = 4,
    },
    [MOTOR_GROUT_ID_GIMBAL1] = {
      .id_feedback = 0x205,
      .id_control = GM6020_CTRL_ID_EXTAND,
      .model = {MOTOR_M3508, MOTOR_M3508, MOTOR_M2006, MOTOR_NONE},
      .num = 2,
    },
    [MOTOR_GROUT_ID_LAUNCHER1] = {
      .id_feedback = 0x209,
      .id_control = M3508_M2006_CTRL_ID_EXTAND,
      .model = {MOTOR_GM6020, MOTOR_GM6020, MOTOR_NONE, MOTOR_NONE},
      .num = 3,
    },
  },
}; /* param_hero */

/* static const config_robot_param_t param_xxx; */

static const config_pilot_cfg_t cfg_qs = {
  .param = {
    .sens_mouse = 0.06f,
    .sens_stick = 6.0f,
    .key_map[CMD_BEHAVIOR_FORE] = {CMD_ACTIVE_PRESSED, CMD_KEY_W},
    .key_map[CMD_BEHAVIOR_BACK] = {CMD_ACTIVE_PRESSED, CMD_KEY_S},
    .key_map[CMD_BEHAVIOR_LEFT] = {CMD_ACTIVE_PRESSED, CMD_KEY_A},
    .key_map[CMD_BEHAVIOR_RIGHT] = {CMD_ACTIVE_PRESSED, CMD_KEY_D},
    .key_map[CMD_BEHAVIOR_ACCELERATE] = {CMD_ACTIVE_PRESSED, CMD_KEY_SHIFT},
    .key_map[CMD_BEHAVIOR_DECELEBRATE] = {CMD_ACTIVE_PRESSED, CMD_KEY_CTRL},
    .key_map[CMD_BEHAVIOR_FIRE] = {CMD_ACTIVE_PRESSED, CMD_KEY_L_CLICK},
    .key_map[CMD_BEHAVIOR_FIRE_MODE] = {CMD_ACTIVE_PRESSING, CMD_KEY_R_CLICK},
    .key_map[CMD_BEHAVIOR_FOLLOWGIMBAL35] = {CMD_ACTIVE_PRESSING, CMD_KEY_E},
    .key_map[CMD_BEHAVIOR_OPENCOVER] =  {CMD_ACTIVE_PRESSING, CMD_KEY_F},
    .key_map[CMD_BEHAVIOR_REVTRIG] = {CMD_ACTIVE_PRESSING, CMD_KEY_R},
    .key_map[CMD_BEHAVIOR_ROTOR] = {CMD_ACTIVE_PRESSING, CMD_KEY_G},
    .key_map[CMD_BEHAVIOR_AUTOAIM] = {CMD_ACTIVE_PRESSING, CMD_KEY_C},
    .move = {
      .sense_norm = 0.8f,
      .sense_fast = 1.25f,
      .sense_slow = 0.8f,
    },
  },
  .screen = {
    .height = 1080,
    .width = 1920,
  },
};

static const config_pilot_cfg_t cfg_zyma = {
  .param = {
    .sens_mouse = 0.06f,
    .sens_stick = 6.0f,
    .key_map[CMD_BEHAVIOR_FORE] = {CMD_ACTIVE_PRESSED, CMD_KEY_W},
    .key_map[CMD_BEHAVIOR_BACK] = {CMD_ACTIVE_PRESSED, CMD_KEY_S},
    .key_map[CMD_BEHAVIOR_LEFT] = {CMD_ACTIVE_PRESSED, CMD_KEY_A},
    .key_map[CMD_BEHAVIOR_RIGHT] = {CMD_ACTIVE_PRESSED, CMD_KEY_D},
    .key_map[CMD_BEHAVIOR_ACCELERATE] = {CMD_ACTIVE_PRESSED, CMD_KEY_SHIFT},
    .key_map[CMD_BEHAVIOR_DECELEBRATE] = {CMD_ACTIVE_PRESSED, CMD_KEY_CTRL},
    .key_map[CMD_BEHAVIOR_FIRE] = {CMD_ACTIVE_PRESSED, CMD_KEY_L_CLICK},
    .key_map[CMD_BEHAVIOR_FIRE_MODE] = {CMD_ACTIVE_PRESSING, CMD_KEY_R_CLICK},
    .key_map[CMD_BEHAVIOR_FOLLOWGIMBAL35] = {CMD_ACTIVE_PRESSING, CMD_KEY_E},
    .key_map[CMD_BEHAVIOR_OPENCOVER] =  {CMD_ACTIVE_PRESSING, CMD_KEY_F},
    .key_map[CMD_BEHAVIOR_REVTRIG] = {CMD_ACTIVE_PRESSING, CMD_KEY_R},
    .key_map[CMD_BEHAVIOR_ROTOR] = {CMD_ACTIVE_PRESSING, CMD_KEY_G},
    .key_map[CMD_BEHAVIOR_AUTOAIM] = {CMD_ACTIVE_PRESSING, CMD_KEY_C},
    .move = {
      .sense_norm = 0.8f,
      .sense_fast = 1.25f,
      .sense_slow = 0.8f,
    },
  },
  .screen = {
    .height = 1080,
    .width = 1920,
  },
};

/* static const config_pilot_cfg_t cfg_xx; */

/* clang-format on */

static const config_robot_param_map_t robot_param_map[] = {
    {"default", &param_default},
    {"infantry", &param_default},
    {"hero", &param_hero},
    // {"engineer", &param_engineer},
    // {"drone", &param_drone},
    // {"sentry", &param_sentry},
    /* {"xxx", &param_xxx}, */
    {NULL, NULL},
};

static const config_pilot_cfg_map_t pilot_cfg_map[] = {
    {"qs", &cfg_qs},
    {"zyma", &cfg_zyma},
    /* {"xx", &cfg_xx}, */
    {NULL, NULL},
};

/**
 * @brief 从Flash读取配置信息
 *
 * @param cfg 配置信息
 */
void config_get(config_t *cfg) {
  BSP_Flash_ReadBytes(CONFIG_BASE_ADDRESS, (uint8_t *)cfg, sizeof(*cfg));

  cfg->pilot_cfg = config_get_pilot_cfg(cfg->pilot_cfg_name);
  cfg->robot_param = config_get_robot_param(cfg->robot_param_name);

  /* 防止第一次烧写后访问NULL指针 */
  if (cfg->robot_param == NULL) cfg->robot_param = &param_default;
  if (cfg->pilot_cfg == NULL) cfg->pilot_cfg = &cfg_qs;

  /* 防止第一次烧写后出现nan */
  if (isnanf(cfg->cali.bmi088.gyro_offset.x)) {
    memset(&(cfg->cali), 0, sizeof(cfg->cali));
  }

  if (isnanf(cfg->gimbal_mech_zero.pit)) {
    memset(&(cfg->gimbal_mech_zero), 0, sizeof(cfg->gimbal_mech_zero));
  }

  if (isnanf(cfg->gimbal_limit)) {
    cfg->gimbal_limit = 1.0f;
  }

  /* 确保配置有效，无内在冲突 */
  ASSERT(cfg->robot_param->chassis.reverse.yaw ==
         cfg->robot_param->gimbal.reverse.yaw);
}

/**
 * @brief 将配置信息写入Flash
 *
 * @param cfg 配置信息
 */
void config_set(config_t *cfg) {
  vTaskSuspendAll();
  BSP_Flash_EraseSector(11);
  BSP_Flash_WriteBytes(CONFIG_BASE_ADDRESS, (uint8_t *)cfg, sizeof(*cfg));
  xTaskResumeAll();
}

/**
 * @brief 通过机器人参数名称获取机器人参数的指针
 *
 * @param robot_param_name 机器人参数名称
 * @return const config_robot_param_t* 机器人参数的指针
 */
const config_robot_param_t *config_get_robot_param(
    const char *robot_param_name) {
  ASSERT(robot_param_name);
  for (size_t j = 0; robot_param_map[j].name != NULL; j++) {
    if (strcmp(robot_param_map[j].name, robot_param_name) == 0) {
      return robot_param_map[j].param;
    }
  }
  return NULL; /* No match. */
}

/**
 * @brief 通过操作手配置名称获取操作手配置的指针
 *
 * @param pilot_cfg_name 操作手配置名称
 * @return const config_pilot_cfg_t* 操作手配置的指针
 */
const config_pilot_cfg_t *config_get_pilot_cfg(const char *pilot_cfg_name) {
  ASSERT(pilot_cfg_name);
  for (size_t j = 0; pilot_cfg_map[j].name != NULL; j++) {
    if (strcmp(pilot_cfg_map[j].name, pilot_cfg_name) == 0) {
      return pilot_cfg_map[j].param;
    }
  }
  return NULL; /* No match. */
}

const config_pilot_cfg_map_t *config_get_pilot_name_map(void) {
  return pilot_cfg_map;
}

const config_robot_param_map_t *config_get_robot_name_map(void) {
  return robot_param_map;
}
