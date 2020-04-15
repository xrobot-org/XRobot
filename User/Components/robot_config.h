/* 
	
*/

#pragma once

#include <stdint.h>


#include "chassis.h"
#include "gimbal.h"
#include "shoot.h"


typedef enum {
	ROBOT_CONFIG_MODEL_INFANTRY,
	ROBOT_CONFIG_MODEL_HERO,
	ROBOT_CONFIG_MODEL_ENGINEER,
	ROBOT_CONFIG_MODEL_DRONE,
	ROBOT_CONFIG_MODEL_SENTRY,
} RobotConfig_Model_t;

typedef struct {
	struct {
		int place_holder;
	}chassis;
	
	struct {
		int place_holder;
	}gimbal;
	
	struct {
		int place_holder;
	}shoot;
} RobotConfig_t;

const RobotConfig_t *RobotConfig_Get(RobotConfig_Model_t model);
