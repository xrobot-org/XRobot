#include "config.h"

static PID_Params_t infantry_chassis_pid_array[4] = {{
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

static const Config_Robot_t config_infantry = {
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
			.pid = {{
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
			.fric_pid_param = {{
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
static const Config_Robot_t config_hero;
static const Config_Robot_t config_engineer;
static const Config_Robot_t config_drone;
static const Config_Robot_t config_sentry;
		
static const Config_User_t user_default = {
	.param = {
		.cmd = {
			.sens_mouse = 0.5f,
			.sens_rc = 0.5f,
		},
		
	},
};

const Config_Robot_t *Config_GetRobot(Config_Model_t model) {
	switch (model) {
		case CONFIG_ROBOT_MODEL_INFANTRY:
			return &config_infantry;
		case CONFIG_ROBOT_MODEL_HERO:
			return &config_hero;
		case CONFIG_ROBOT_MODEL_ENGINEER:
			return &config_engineer;
		case CONFIG_ROBOT_MODEL_DRONE:
			return &config_drone;
		case CONFIG_ROBOT_MODEL_SENTRY:
			return &config_sentry;
		default:
			return NULL;
	}
}

const Config_User_t *Config_GetUser(Config_UserName_t user) {
	switch (user) {
		case CONFIG_USER_DEFAULT:
			return &user_default;
		default:
			return NULL;
	}
}

const Config_Robot_t **RobotConfig_GetDefault(void) {
	return NULL;
}
