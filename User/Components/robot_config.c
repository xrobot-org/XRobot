/* 
	Modified from https://github.com/PX4/Firmware/blob/master/src/lib/pid/pid.cpp

*/


#include "robot_config.h"
static const RobotConfig_t config_infantry = {0
	
};

static const RobotConfig_t config_hero;
static const RobotConfig_t config_engineer;
static const RobotConfig_t config_drone;

static const RobotConfig_t config_sentry= {0
	
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
