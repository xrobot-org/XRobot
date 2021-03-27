/*
 * 配置相关
 */

#include "config.h"

#include <stdint.h>
#include <string.h>

#include "bsp/flash.h"

#define CONFIG_BASE_ADDRESS (ADDR_FLASH_SECTOR_11)

/* clang-format off */
#ifdef DEBUG
Config_RobotParam_t param_default = {
#else
static const Config_RobotParam_t param_default = {
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
      .k = 2.0f,
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
        /* GIMBAL_PID_YAW_OMEGA_IDX */
        .k = 0.225326675239841f,
        .p = 1.99995375f,
        .i = 30.72000008f,
        .d = 0.00589999976f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = -1.0f,
      }, {
        /* GIMBAL_PID_YAW_ANGLE_IDX */
        .k = 9.58196376868962f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 0.0f,
        .out_limit = 1000.0f,
        .d_cutoff_freq = -1.0f,
        .range = M_2PI,
      }, {
        /* GIMBAL_PID_PIT_OMEGA_IDX */
        .k = 0.12f,
        .p = 1.0f,
        .i = 0.119999997f,
        .d = 0.00499999989f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = -1.0f,
      }, {
        /* GIMBAL_PID_PIT_ANGLE_IDX */
        .k = 7.0f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 0.0f,
        .out_limit = 1000.0f,
        .d_cutoff_freq = -1.0f,
        .range = M_2PI,
      },
    }, /* pid */

		.pitch_travel_rad = 1.0f,
    
    .low_pass_cutoff_freq = {
      .out = -1.0f,
      .gyro = 1000.0f,
    },

    .reverse = {
      .yaw = false,
      .pit = false,
    },
  }, /* gimbal */

  .shoot = { /* 射击模块参数 */
            
    .fric_pid_param = {
      .k = 0.001f,
      .p = 1.0f,
      .i = 0.5f,
      .d = 0.5f,
      .i_limit = 0.5f,
      .out_limit = 0.5f,
      .d_cutoff_freq = -1.0f,
    },
    .trig_pid_param = {
      .k = 4.0f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.037f,
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
    .bullet_speed_scaler = -50.0f,
    .bullet_speed_bias = 1.0f,
    .num_trig_tooth = 8.0f,
    .fric_radius_m = 0.03f,
    .cover_open_duty = 0.125f,
    .cover_close_duty = 0.075f,
  }, /* shoot */
  .can={
    .chassis = {
      .can = CAN1_OCP,
    },
    .gimbal = {
      .can = CAN1_OCP,
    },
    .shoot = {
      .can = CAN2_OCP,
    },       
    .cap = {
      .can = CAN1_OCP,
    },   
  }, /* can */
}; /* param_default */

static const Config_RobotParam_t param_hero = {
  .model = ROBOT_MODEL_HERO,

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
      .k = 0.0f,
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
        /* GIMBAL_PID_YAW_OMEGA_IDX */
        .k = 0.245326675239841f,
        .p = 1.0f,
        .i = 28.274136896f,
        .d = 0.007071387562335f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = -1.0f,
      }, {
        /* GIMBAL_PID_YAW_ANGLE_IDX */
        .k = 9.58196376868962f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 0.0f,
        .out_limit = 1000.0f,
        .d_cutoff_freq = -1.0f,
        .range = M_2PI,
      }, {
        /* GIMBAL_PID_PIT_OMEGA_IDX */
        .k = 0.15f,
        .p = 1.0f,
        .i = 0.05f,
        .d = 0.003f,
        .i_limit = 1.0f,
        .out_limit = 1.0f,
        .d_cutoff_freq = -1.0f,
        .range = -1.0f,
      }, {
        /* GIMBAL_PID_PIT_ANGLE_IDX */
        .k = 7.0f,
        .p = 1.0f,
        .i = 0.0f,
        .d = 0.0f,
        .i_limit = 0.0f,
        .out_limit = 1000.0f,
        .d_cutoff_freq = -1.0f,
        .range = M_2PI,
      },
    }, /* pid */

    .low_pass_cutoff_freq = {
      .out = -1.0f,
      .gyro = 1000.0f,
    },

    .reverse = {
      .yaw = true,
      .pit = true,
    },
  }, /* gimbal */

  .shoot = { /* 射击模块参数 */
            
    .fric_pid_param = {
      .k = 0.001f,
      .p = 1.0f,
      .i = 0.5f,
      .d = 0.5f,
      .i_limit = 0.5f,
      .out_limit = 0.5f,
      .d_cutoff_freq = -1.0f,
    },
    .trig_pid_param = {
      .k = 4.0f,
      .p = 1.0f,
      .i = 0.0f,
      .d = 0.037f,
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
    .bullet_speed_scaler = -50.0f,
    .bullet_speed_bias = 1.0f,
    .num_trig_tooth = 6.0f,
    .fric_radius_m = 0.03f,
  }, /* shoot */
}; /* param_hero */      

static const Config_RobotParam_t param_engineer;
static const Config_RobotParam_t param_drone;
static const Config_RobotParam_t param_sentry;
/* static const Config_RobotParam_t param_xxx; */

static const Config_PilotCfg_t cfg_qs = {
  .param = {
		.sens_mouse = 0.5f,
		.sens_rc = 2.0f,
    .map = {
      .Key_Mapping[CMD_BEHAVIOR_FORE] = CMD_KEY_W,
      .Key_Mapping[CMD_BEHAVIOR_BACK] = CMD_KEY_S,
      .Key_Mapping[CMD_BEHAVIOR_LEFT] = CMD_KEY_A,
      .Key_Mapping[CMD_BEHAVIOR_RIGHT] = CMD_KEY_D,
      .Key_Mapping[CMD_BEHAVIOR_ACCELERATE] = CMD_KEY_SHIFT,
      .Key_Mapping[CMD_BEHAVIOR_DECELEBRATE] = CMD_KEY_CTRL,
      .Key_Mapping[CMD_BEHAVIOR_FIRE] = CMD_L_CLICK,
      .Key_Mapping[CMD_BEHAVIOR_BUFF] = CMD_KEY_E,
      .Key_Mapping[CMD_BEHAVIOR_AUTOAIM] = CMD_R_CLICK,
      .Key_Mapping[CMD_BEHAVIOR_OPENCOVER] =  CMD_KEY_F,
    },
    .move = {
      .move_sense = 0.8f,
      .move_fast_sense = 1.2f,
      .move_slow_sense = 0.8f,		
    },       
  },
};

/* static const Config_PilotCfg_t cfg_xx; */

/* clang-format on */

static const Config_RobotParamMap_t robot_param_map[] = {
    {"default", &param_default},
    {"infantry", &param_default},
    {"hero", &param_hero},
    {"engineer", &param_engineer},
    {"drone", &param_drone},
    {"sentry", &param_sentry},
    /* {"xxx", &param_xxx}, */
    {NULL, NULL},
};

static const Config_PilotCfgMap_t pilot_cfg_map[] = {
    {"qs", &cfg_qs},
    /* {"xx", &cfg_xx}, */
    {NULL, NULL},
};

/**
 * \brief 从Flash读取配置信息
 *
 * \param cfg 配置信息
 */
void Config_Get(Config_t *cfg) {
  BSP_Flash_ReadBytes(CONFIG_BASE_ADDRESS, (uint8_t *)cfg, sizeof(*cfg));
  cfg->pilot_cfg = Config_GetPilotCfg(cfg->pilot_cfg_name);
  cfg->robot_param = Config_GetRobotParam(cfg->robot_param_name);
  /* 防止第一次烧写后访问NULL指针 */
  if (cfg->robot_param == NULL) cfg->robot_param = &param_default;
  if (cfg->pilot_cfg == NULL) cfg->pilot_cfg = &cfg_qs;
  /* 防止擦除后全为1 */
  if ((uint32_t)(cfg->robot_param) == UINT32_MAX)
    cfg->robot_param = &param_default;
  if ((uint32_t)(cfg->pilot_cfg) == UINT32_MAX) cfg->pilot_cfg = &cfg_qs;
}

/**
 * \brief 将配置信息写入Flash
 *
 * \param cfg 配置信息
 */
void Config_Set(Config_t *cfg) {
  osKernelLock();
  BSP_Flash_EraseSector(11);
  BSP_Flash_WriteBytes(CONFIG_BASE_ADDRESS, (uint8_t *)cfg, sizeof(*cfg));
  osKernelUnlock();
}

/**
 * @brief 通过机器人参数名称获取机器人参数的指针
 *
 * @param robot_param_name 机器人参数名称
 * @return const Config_RobotParam_t* 机器人参数的指针
 */
const Config_RobotParam_t *Config_GetRobotParam(const char *robot_param_name) {
  if (robot_param_name == NULL) return NULL;
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
 * @return const Config_PilotCfg_t* 操作手配置的指针
 */
const Config_PilotCfg_t *Config_GetPilotCfg(const char *pilot_cfg_name) {
  if (pilot_cfg_name == NULL) return NULL;
  for (size_t j = 0; pilot_cfg_map[j].name != NULL; j++) {
    if (strcmp(pilot_cfg_map[j].name, pilot_cfg_name) == 0) {
      return pilot_cfg_map[j].param;
    }
  }
  return NULL; /* No match. */
}

const Config_PilotCfgMap_t *Config_GetPilotNameMap(void) {
  return pilot_cfg_map;
}

const Config_RobotParamMap_t *Config_GetRobotNameMap(void) {
  return robot_param_map;
}