/* 
	控制命令
*/

#pragma once

#include "user_math.h"
#include "ahrs.h"


typedef struct {
	MoveVector_t move_vector;
	AHRS_Eulr_t eulr_gimbal;
	float32_t bullet_speed;
	float32_t shoot_freq_hz;
} Command_t;


/*  
	CHASSIS_MODE_RELAX: No force applied. For all robot when power on.
	CHASSIS_MODE_BREAK: Set to zero speed. Force applied. For all robot when break.
	CHASSIS_MODE_FOLLOW_GIMBAL: Follow gimbal by follow encoder. For infantry, hero and engineer.
	CHASSIS_MODE_ROTOR: Constantly rotating. For infantry and hero.
	CHASSIS_MODE_INDENPENDENT: Run independently. For sentry and drone.
	CHASSIS_MODE_OPEN: Direct apply force without pid control. For TEST only.
*/
typedef enum {
	CHASSIS_MODE_RELAX, 
	CHASSIS_MODE_BREAK,
	CHASSIS_MODE_FOLLOW_GIMBAL,
	CHASSIS_MODE_ROTOR,
	CHASSIS_MODE_INDENPENDENT,
	CHASSIS_MODE_OPEN,
} Chassis_Mode_t;
