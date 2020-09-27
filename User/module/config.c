#include "config.h"

#include <string.h>

#include "bsp/flash.h"

#define CONFIG_BASE_ADDRESS (ADDR_FLASH_END - sizeof(Config_t))

#ifdef DEBUG
Config_Robot_t cfg_infantry = {
#else
static const Config_Robot_t cfg_infantry = {
#endif
    .model = ROBOT_MODEL_INFANTRY,

    /* 对应模块的参数 */
    .param = {
      .chassis = { /* 底盘模块参数 */ 
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
            .k = 0.001f,
            .p = 1.0f,
            .i = 0.0f,
            .d = 0.0f,
            .i_limit = 1.0f,
            .out_limit = 1.0f,
            .d_cutoff_freq = -1.0f,
            .range = 2 * M_PI,
          },
          .low_pass_cutoff_freq = {
            .in = -1.0f,
            .out = -1.0f,
          },
      }, /* chassis */

      .gimbal = { /* 云台模块参数 */
          .pid = {
            {
              /* GIMBAL_PID_YAW_OMEGA_IDX */
              .k = 0.3f,
              .p = 1.0f,
              .i = 0.1f,
              .d = 0.0f,
              .i_limit = 1.0f,
              .out_limit = 1.0f,
              .d_cutoff_freq = -1.0f,
              .range = -1.0f,
            },{
              /* GIMBAL_PID_YAW_ANGLE_IDX */
              .k = 2.8f,
              .p = 1.0f,
              .i = 0.0f,
              .d = 0.0f,
              .i_limit = 0.0f,
              .out_limit = 1000.0f,
              .d_cutoff_freq = -1.0f,
              .range = 2 * M_PI,
            },{
              /* GIMBAL_PID_PIT_OMEGA_IDX */
              .k = 0.15f,
              .p = 1.0f,
              .i = 0.05f,
              .d = 0.003f,
              .i_limit = 1.0f,
              .out_limit = 1.0f,
              .d_cutoff_freq = -1.0f,
              .range = -1.0f,
            },{
              /* GIMBAL_PID_PIT_ANGLE_IDX */
              .k = 7.0f,
              .p = 1.0f,
              .i = 0.0f,
              .d = 0.0f,
              .i_limit = 0.0f,
              .out_limit = 1000.0f,
              .d_cutoff_freq = -1.0f,
              .range = 2 * M_PI,
            },{
              /* GIMBAL_PID_REL_YAW_IDX */
              .k = 0.1f,
              .p = 1.0f,
              .i = 0.01f,
              .d = 0.001f,
              .i_limit = 1.0f,
              .out_limit = 1.0f,
              .d_cutoff_freq = -1.0f,
              .range = 2 * M_PI,
            },{
              /* GIMBAL_PID_REL_PIT_IDX, */
              .k = 0.1f,
              .p = 1.0f,
              .i = 0.01f,
              .d = 0.001f,
              .i_limit = 1.0f,
              .out_limit = 1.0f,
              .d_cutoff_freq = -1.0f,
              .range = 2 * M_PI,
            },
          }, /* pid */
          .low_pass_cutoff_freq = {
            .out = -1.0f,
            .gyro = 1000.0f,
          },
          .encoder_center = {
            .yaw = 3.0f,
            .pit = 3.0f,
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
          .k = 0.001f,
          .p = 1.0f,
          .i = 0.5f,
          .d = 0.5f,
          .i_limit = 0.5f,
          .out_limit = 0.5f,
          .d_cutoff_freq = -1.0f,
          .range = 2 * M_PI,
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
        .bullet_speed_scaler = 1.0f,
        .bullet_speed_bias = 1.0f,
        .num_trig_tooth = 8.0f,
      }, /* shoot */
  }, /* param */
}; /* cfg_infantry */

static const Config_Robot_t cfg_hero;
static const Config_Robot_t cfg_engineer;
static const Config_Robot_t cfg_drone;
static const Config_Robot_t cfg_sentry;

static const Config_Pilot_t user_qs = {
    .param = {
      .cmd = {
        .sens_mouse = 0.5f,
        .sens_rc = 0.01f,
      },
    },
};

void Config_Get(Config_t *cfg) {
  BSP_Flash_ReadBytes(CONFIG_BASE_ADDRESS, (uint8_t *)cfg, sizeof(Config_t));
}

void Config_Set(Config_t *cfg) {
  BSP_Flash_EraseSector(11);
  BSP_Flash_WriteBytes(CONFIG_BASE_ADDRESS, (uint8_t *)cfg, sizeof(Config_t));
}

const Config_Robot_t *Config_GetRobotCfg(Config_RobotModel_t model) {
  switch (model) {
    case ROBOT_MODEL_INFANTRY:
      return &cfg_infantry;
    case ROBOT_MODEL_HERO:
      return &cfg_hero;
    case ROBOT_MODEL_ENGINEER:
      return &cfg_engineer;
    case ROBOT_MODEL_DRONE:
      return &cfg_drone;
    case ROBOT_MODEL_SENTRY:
      return &cfg_sentry;
    case ROBOT_MODEL_NUM:
      /* Default infantry*/
      return &cfg_infantry;
  }
  return &cfg_infantry;
}

const Config_Pilot_t *Config_GetPilotCfg(Config_PilotName_t pilot) {
  switch (pilot) {
    case ROBOT_PILOT_QS:
      return &user_qs;
    case ROBOT_PILOT_NUM:
      /* Default user_qs*/
      return &user_qs;
  }
  return &user_qs;
}

static const struct {
  Config_RobotModel_t model;
  const char *name;
} model_string_map[] = {
    {ROBOT_MODEL_INFANTRY, "Infantry"}, {ROBOT_MODEL_HERO, "Hero"},
    {ROBOT_MODEL_ENGINEER, "Engineer"}, {ROBOT_MODEL_DRONE, "Drone"},
    {ROBOT_MODEL_SENTRY, "Sentry"},     {ROBOT_MODEL_NUM, NULL},
};

static const struct {
  Config_PilotName_t pilot;
  const char *name;
} pilot_string_map[] = {
    {ROBOT_PILOT_QS, "qs"},
    {ROBOT_PILOT_NUM, NULL},
};

Config_RobotModel_t Config_GetModelByName(const char *name) {
  for (int j = 0; model_string_map[j].name != NULL; j++) {
    if (strstr(model_string_map[j].name, name) != NULL) {
      return model_string_map[j].model;
    }
  }
  return ROBOT_MODEL_NUM; /* No match. */
}

Config_PilotName_t Config_GetPilotByName(const char *name) {
  for (int j = 0; pilot_string_map[j].name != NULL; j++) {
    if (strcmp(pilot_string_map[j].name, name) == 0) {
      return pilot_string_map[j].pilot;
    }
  }
  return ROBOT_PILOT_NUM; /* No match. */
}

const char *Config_GetNameByModel(Config_RobotModel_t model) {
  for (int j = 0; model_string_map[j].name != NULL; j++) {
    if (model_string_map[j].model == model) {
      return model_string_map[j].name;
    }
  }
  return "Unknown"; /* No match. */
}

const char *Config_GetNameByPilot(Config_PilotName_t pilot) {
  for (int j = 0; pilot_string_map[j].name != NULL; j++) {
    if (pilot_string_map[j].pilot == pilot) {
      return pilot_string_map[j].name;
    }
  }
  return "Unknown"; /* No match. */
}
