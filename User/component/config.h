/* 
	
*/

#pragma once

#include <stdint.h>

#include "component\cmd.h"

#include "module\chassis.h"
#include "module\gimbal.h"
#include "module\shoot.h"

#define CONFIG_ADDRESS 

typedef enum {
	CONFIG_ROBOT_MODEL_INFANTRY,
	CONFIG_ROBOT_MODEL_HERO,
	CONFIG_ROBOT_MODEL_ENGINEER,
	CONFIG_ROBOT_MODEL_DRONE,
	CONFIG_ROBOT_MODEL_SENTRY,
} Config_Model_t;

typedef enum {
	CONFIG_USER_DEFAULT,
} Config_UserName_t;

typedef struct {
	struct {
		Chassis_Params_t chassis;
		Gimbal_Params_t gimbal;
		Shoot_Params_t shoot;
	} param;
	
	Config_Model_t model;
} Config_Robot_t;

typedef struct {
	struct {
		CMD_Params_t cmd;
	} param;
} Config_User_t;

const Config_Robot_t *Config_GetRobot(Config_Model_t model);
const Config_Robot_t *Config_GetRobotDefalult(void);

const Config_User_t *Config_GetUser(Config_UserName_t user);
