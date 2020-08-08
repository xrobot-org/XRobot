#include "robot.h"

#include <string.h>

#include "bsp/flash.h"

#define CONFIG_BASE_ADDRESS (ADDR_FLASH_END - sizeof(Robot_ID_t))

static const PID_Params_t infantry_chassis_pid_array[4] = {{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.i_limit = 0.5,
		.out_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.i_limit = 0.5,
		.out_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.i_limit = 0.5,
		.out_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.i_limit = 0.5,
		.out_limit = 0.5,
	},
};

static const Robot_Config_t cfg_infantry = {
	.model = ROBOT_MODEL_INFANTRY,
	.param = {
		.chassis = {
			.motor_pid_param = infantry_chassis_pid_array,
			.follow_pid_param = {
				.kp = 0.5,
				.ki = 0.5,
				.kd = 0.5,
				.i_limit = 0.5,
				.out_limit = 0.5,
			},
			.low_pass_cutoff = 100.f,
		}, /* chassis */
		
		.gimbal = {
			.pid = {
				{
					/* GIMBAL_PID_YAW_IN */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_YAW_OUT */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_PIT_IN */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_PIT_OUT */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_REL_YAW */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					/* GIMBAL_PID_REL_PIT, */
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},
			},  /* pid */
			.low_pass_cutoff = 100.f,
		},  /* gimbal */
		
		.shoot = {
			.fric_pid_param = {
				{
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				},{
					.kp = 0.5,
					.ki = 0.5,
					.kd = 0.5,
					.i_limit = 0.5,
					.out_limit = 0.5,
				}
			},
			
			.trig_pid_param = {
				.kp = 0.5,
				.ki = 0.5,
				.kd = 0.5,
				.i_limit = 0.5,
				.out_limit = 0.5,
			},
			
			.low_pass_cutoff = {
				.fric = 100.f,
				.trig = 100.f,
			},
		},  /* shoot */
	},
}; /* cfg_infantry */

static const Robot_Config_t cfg_hero;
static const Robot_Config_t cfg_engineer;
static const Robot_Config_t cfg_drone;
static const Robot_Config_t cfg_sentry;

static const Robot_PilotConfig_t user_qs = {
	.param = {
		.cmd = {
			.sens_mouse = 0.5f,
			.sens_rc = 0.5f,
		},
	},
};

void Robot_GetRobotID(Robot_ID_t *id) {
	BSP_Flash_ReadBytes(CONFIG_BASE_ADDRESS, (uint8_t*)id, sizeof(Robot_ID_t));
}

void Robot_SetRobotID(Robot_ID_t *id) {
	BSP_Flash_EraseSector(11);
	BSP_Flash_WriteBytes(CONFIG_BASE_ADDRESS, (uint8_t*)id, sizeof(Robot_ID_t));
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
	const char* name;
} model_string_map[] = {
	{ROBOT_MODEL_INFANTRY, "Infantry"},
	{ROBOT_MODEL_HERO, "Hero"},
	{ROBOT_MODEL_ENGINEER, "Engineer"},
	{ROBOT_MODEL_DRONE, "Drone"},
	{ROBOT_MODEL_SENTRY, "Sentry"},
	{ROBOT_MODEL_NUM, NULL},
};

static const struct {
	Robot_Pilot_t pilot;
	const char* name;
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
