#include "robot.h"

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

static const Robot_Config_t config_infantry = {
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
		},
		
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
			},
			.low_pass_cutoff = 100.f,
		},
		
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
			.low_pass_cutoff = 100.f,
		},
	},
};
static const Robot_Config_t config_hero;
static const Robot_Config_t config_engineer;
static const Robot_Config_t config_drone;
static const Robot_Config_t config_sentry;

static const Robot_UserConfig_t user_qs = {
	.param = {
		.cmd = {
			.sens_mouse = 0.5f,
			.sens_rc = 0.5f,
		},
		
	},
};

static void Robot_GetRobotID(Robot_ID_t *id) {
	BSP_Flash_ReadBytes(CONFIG_BASE_ADDRESS, (uint8_t*)id, sizeof(Robot_ID_t));
}

const Robot_Config_t *Robot_GetConfigDefault(void) {
	Robot_ID_t id;
	Robot_GetRobotID(&id);
	switch (id.model) {
		case ROBOT_MODEL_INFANTRY:
			return &config_infantry;
		case ROBOT_MODEL_HERO:
			return &config_hero;
		case ROBOT_MODEL_ENGINEER:
			return &config_engineer;
		case ROBOT_MODEL_DRONE:
			return &config_drone;
		case ROBOT_MODEL_SENTRY:
			return &config_sentry;
	}
	return &config_infantry;
}

const Robot_UserConfig_t *Robot_GetUserConfigDefault(void) {
	Robot_ID_t id;
	Robot_GetRobotID(&id);
	switch (id.user) {
		case ROBOT_USER_QS:
			return &user_qs;
	}
	return &user_qs;
}

void Robot_SetRobotID(Robot_ID_t *id) {
	BSP_Flash_WriteBytes(CONFIG_BASE_ADDRESS, (uint8_t*)id, sizeof(Robot_ID_t));
}
