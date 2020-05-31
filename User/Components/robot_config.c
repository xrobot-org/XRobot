#include "robot_config.h"

static PID_Params_t infantry_chassis_motor_pid_array[4] = {{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.integral_limit = 0.5,
		.output_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.integral_limit = 0.5,
		.output_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.integral_limit = 0.5,
		.output_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.integral_limit = 0.5,
		.output_limit = 0.5,
	},
};

static const RobotConfig_t config_infantry = {
	.chassis_param = {
		.motor_pid_param = infantry_chassis_motor_pid_array,
		.follow_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
	},
	
	.gimbal_param = {
		.yaw_inner_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
		.yaw_outer_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
		
		.pit_inner_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
		.pit_outer_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
	},
	
	.shoot_param = {
		.fric_pid_param = {{
				.kp = 0.5,
				.ki = 0.5,
				.kd = 0.5,
				.integral_limit = 0.5,
				.output_limit = 0.5,
			},{
				.kp = 0.5,
				.ki = 0.5,
				.kd = 0.5,
				.integral_limit = 0.5,
				.output_limit = 0.5,
			}
		},
		
		.trig_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
	},
};

static const RobotConfig_t config_hero;
static const RobotConfig_t config_engineer;
static const RobotConfig_t config_drone;

static PID_Params_t sentry_chassis_motor_pid[2] = {{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.integral_limit = 0.5,
		.output_limit = 0.5,
	},{
		.kp = 0.5,
		.ki = 0.5,
		.kd = 0.5,
		.integral_limit = 0.5,
		.output_limit = 0.5,
	},
};

static const RobotConfig_t config_sentry = {
	.chassis_param = {
		.motor_pid_param = sentry_chassis_motor_pid,
		.follow_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit= 0.5,
			.output_limit = 0.5,
		},
	},
	
	.gimbal_param = {
		.yaw_inner_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
		.yaw_outer_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
		
		.pit_inner_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
		.pit_outer_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
	},
	
	.shoot_param = {
		.fric_pid_param = {{
				.kp = 0.5,
				.ki = 0.5,
				.kd = 0.5,
				.integral_limit = 0.5,
				.output_limit = 0.5,
			},{
				.kp = 0.5,
				.ki = 0.5,
				.kd = 0.5,
				.integral_limit = 0.5,
				.output_limit = 0.5,
			}
		},
		
		.trig_pid_param = {
			.kp = 0.5,
			.ki = 0.5,
			.kd = 0.5,
			.integral_limit = 0.5,
			.output_limit = 0.5,
		},
	},
};

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
