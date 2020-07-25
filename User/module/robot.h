/* 
	
*/

#pragma once

#include <stdint.h>

#include "component\cmd.h"

#include "module\chassis.h"
#include "module\gimbal.h"
#include "module\shoot.h"

typedef enum {
	ROBOT_MODEL_INFANTRY = 0,
	ROBOT_MODEL_HERO,
	ROBOT_MODEL_ENGINEER,
	ROBOT_MODEL_DRONE,
	ROBOT_MODEL_SENTRY,
} Robot_Model_t;

typedef enum {
	ROBOT_PILOT_QS = 0,
} Robot_Pilot_t;

typedef struct {
	Robot_Model_t model;
	
	struct {
		Chassis_Params_t chassis;
		Gimbal_Params_t gimbal;
		Shoot_Params_t shoot;
	} param;
	
} Robot_Config_t;

typedef struct {
	struct {
		CMD_Params_t cmd;
	} param;
	
	struct {
		void (*MapChassis)(void);
		void (*MapGimbal)(void);
		void (*MapShoot)(void);
	} key_map;
} Robot_PilotConfig_t;

typedef struct {
	Robot_Model_t model;
	Robot_Pilot_t pilot;
} Robot_ID_t;

void Robot_GetRobotID(Robot_ID_t *id);
void Robot_SetRobotID(Robot_ID_t *id);
const Robot_Config_t *Robot_GetConfigDefault(void);
const Robot_PilotConfig_t *Robot_GetPilotConfigDefault(void);
