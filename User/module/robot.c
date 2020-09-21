#include "robot.h"

#include <string.h>

#include "bsp/flash.h"

#define CONFIG_BASE_ADDRESS (ADDR_FLASH_END - sizeof(Robot_ID_t))

static const Robot_Config_t cfg_infantry = {
    .model = ROBOT_MODEL_INFANTRY,

    /* 对应模块的参数 */
    .param =
        {
            .chassis = /* 底盘模块参数 */
            {
                .motor_pid_param =
                    {
                        .kp = 0.001f,
                        .ki = 0.0f,
                        .kd = 0.0f,
                        .i_limit = 1.0f,
                        .out_limit = 1.0f,
                    },

                .follow_pid_param =
                    {
                        .kp = 0.01f,
                        .ki = 0.0f,
                        .kd = 0.0f,
                        .i_limit = 1.0f,
                        .out_limit = 1.0f,
                    },
                .low_pass_cutoff_freq = -1.0f,
            }, /* chassis */

            .gimbal = /* 云台模块参数 */
            {
                .pid =
                    {
                        {
                            /* GIMBAL_PID_YAW_OMEGA_IDX */
                            .kp = 0.5f,
                            .ki = 0.0f,
                            .kd = 0.0f,
                            .i_limit = 1.0f,
                            .out_limit = 1.0f,
                        },
                        {
                            /* GIMBAL_PID_YAW_ANGLE_IDX */
                            .kp = 0.01f,
                            .ki = 0.0f,
                            .kd = 0.0f,
                            .i_limit = 1.0f,
                            .out_limit = 1.0f,
                        },
                        {
                            /* GIMBAL_PID_PIT_OMEGA_IDX */
                            .kp = 0.5f,
                            .ki = 0.0f,
                            .kd = 0.0f,
                            .i_limit = 1.0f,
                            .out_limit = 1.0f,
                        },
                        {
                            /* GIMBAL_PID_PIT_ANGLE_IDX */
                            .kp = 0.01f,
                            .ki = 0.0f,
                            .kd = 0.0f,
                            .i_limit = 1.0f,
                            .out_limit = 1.0f,
                        },
                        {
                            /* GIMBAL_PID_REL_YAW_IDX */
                            .kp = 0.2f,
                            .ki = 0.1f,
                            .kd = 0.1f,
                            .i_limit = 0.5f,
                            .out_limit = 0.5f,
                        },
                        {
                            /* GIMBAL_PID_REL_PIT_IDX, */
                            .kp = 0.2f,
                            .ki = 0.1f,
                            .kd = 0.1f,
                            .i_limit = 0.5f,
                            .out_limit = 0.5f,
                        },
                    }, /* pid */
                .out_low_pass_cutoff_freq = -1.0f,
                .gyro_low_pass_cutoff_freq = 1000.0f,
                .encoder_center =
                    {
                        .yaw = 3.0f,
                        .pit = 3.0f,
                    },

            }, /* gimbal */

            .shoot = /* 射击模块参数 */
            {
                .fric_pid_param =
                    {
                        {
                            .kp = 0.5f,
                            .ki = 0.5f,
                            .kd = 0.5f,
                            .i_limit = 0.5f,
                            .out_limit = 0.5f,
                        },
                        {
                            .kp = 0.5f,
                            .ki = 0.5f,
                            .kd = 0.5f,
                            .i_limit = 0.5f,
                            .out_limit = 0.5f,
                        },
                    },

                .trig_pid_param =
                    {
                        .kp = 0.5f,
                        .ki = 0.5f,
                        .kd = 0.5f,
                        .i_limit = 0.5f,
                        .out_limit = 0.5f,
                    },

                .low_pass_cutoff_freq =
                    {
                        .fric = -1.0f,
                        .trig = -1.0f,
                    },

                .bullet_speed_scaler = 1.0f,
                .bullet_speed_bias = 1.0f,
                .num_trig_tooth = 8.0f,
            }, /* shoot */
        },
}; /* cfg_infantry */

static const Robot_Config_t cfg_hero;
static const Robot_Config_t cfg_engineer;
static const Robot_Config_t cfg_drone;
static const Robot_Config_t cfg_sentry;

static const Robot_PilotConfig_t user_qs = {
    .param =
        {
            .cmd =
                {
                    .sens_mouse = 0.5f,
                    .sens_rc = 0.01f,
                },
        },
};

void Robot_GetRobotID(Robot_ID_t *id) {
  BSP_Flash_ReadBytes(CONFIG_BASE_ADDRESS, (uint8_t *)id, sizeof(Robot_ID_t));
}

void Robot_SetRobotID(Robot_ID_t *id) {
  BSP_Flash_EraseSector(11);
  BSP_Flash_WriteBytes(CONFIG_BASE_ADDRESS, (uint8_t *)id, sizeof(Robot_ID_t));
}

const Robot_Config_t *Robot_GetConfig(Robot_Model_t model) {
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

const Robot_PilotConfig_t *Robot_GetPilotConfig(Robot_Pilot_t pilot) {
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
  Robot_Model_t model;
  const char *name;
} model_string_map[] = {
    {ROBOT_MODEL_INFANTRY, "Infantry"}, {ROBOT_MODEL_HERO, "Hero"},
    {ROBOT_MODEL_ENGINEER, "Engineer"}, {ROBOT_MODEL_DRONE, "Drone"},
    {ROBOT_MODEL_SENTRY, "Sentry"},     {ROBOT_MODEL_NUM, NULL},
};

static const struct {
  Robot_Pilot_t pilot;
  const char *name;
} pilot_string_map[] = {
    {ROBOT_PILOT_QS, "qs"},
    {ROBOT_PILOT_NUM, NULL},
};

Robot_Model_t Robot_GetModelByName(const char *name) {
  for (int j = 0; model_string_map[j].name != NULL; j++) {
    if (strstr(model_string_map[j].name, name) != NULL) {
      return model_string_map[j].model;
    }
  }
  return ROBOT_MODEL_NUM; /* No match. */
}

Robot_Pilot_t Robot_GetPilotByName(const char *name) {
  for (int j = 0; pilot_string_map[j].name != NULL; j++) {
    if (strcmp(pilot_string_map[j].name, name) == 0) {
      return pilot_string_map[j].pilot;
    }
  }
  return ROBOT_PILOT_NUM; /* No match. */
}

const char *Robot_GetNameByModel(Robot_Model_t model) {
  for (int j = 0; model_string_map[j].name != NULL; j++) {
    if (model_string_map[j].model == model) {
      return model_string_map[j].name;
    }
  }
  return "Unknown"; /* No match. */
}

const char *Robot_GetNameByPilot(Robot_Pilot_t pilot) {
  for (int j = 0; pilot_string_map[j].name != NULL; j++) {
    if (pilot_string_map[j].pilot == pilot) {
      return pilot_string_map[j].name;
    }
  }
  return "Unknown"; /* No match. */
}
