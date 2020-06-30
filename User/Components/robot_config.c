#include "robot_config.h"

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

static const RobotConfig_t config_infantry = {
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

static const RobotConfig_t config_hero;
static const RobotConfig_t config_engineer;
static const RobotConfig_t config_drone;
static const RobotConfig_t config_sentry;;

const RobotConfig_t *RobotConfig_Get(RobotConfig_Model_t model) {
	switch (model) {
		case ROBOT_CONFIG_MODEL_INFANTRY:
			return &config_infantry;
		case ROBOT_CONFIG_MODEL_HERO:
			return &config_hero;
		case ROBOT_CONFIG_MODEL_ENGINEER:
			return &config_engineer;
		case ROBOT_CONFIG_MODEL_DRONE:
			return &config_drone;
		case ROBOT_CONFIG_MODEL_SENTRY:
			return &config_sentry;
		default:
			return NULL;
	}
}
