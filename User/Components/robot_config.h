/* 
	
*/

#pragma once

#include <stdint.h>

#include "chassis.h"
#include "gimbal.h"
#include "shoot.h"

#define ROBOT_CONFIG_ADDRESS 

typedef enum {
	ROBOT_CONFIG_MODEL_INFANTRY,
	ROBOT_CONFIG_MODEL_HERO,
	ROBOT_CONFIG_MODEL_ENGINEER,
	ROBOT_CONFIG_MODEL_DRONE,
	ROBOT_CONFIG_MODEL_SENTRY,
} RobotConfig_Model_t;

typedef struct {
	struct {
		const Chassis_Params_t chassis;
		const Gimbal_Params_t gimbal;
		const Shoot_Params_t shoot;
	} param;
	
	/* Xxx_Params_t xxx_param; */
} RobotConfig_t;

const RobotConfig_t *RobotConfig_Get(RobotConfig_Model_t model);
