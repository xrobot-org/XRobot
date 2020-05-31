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
	const Chassis_Params_t chassis_param;
	const Gimbal_Params_t gimbal_param;
	const Shoot_Params_t shoot_param;
	
	/* Xxx_Params_t xxx_param; */
} RobotConfig_t;

const RobotConfig_t *RobotConfig_Get(RobotConfig_Model_t model);
