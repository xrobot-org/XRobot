#pragma once


/* Includes ------------------------------------------------------------------*/
#include "pid.h"

/* Exported constants --------------------------------------------------------*/
/* Exported defines ----------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/*  
	CHASSIS_MODE_RELAX: Not force applied.
	CHASSIS_MODE_BREAK: Set to zero speed. Force applied.
	CHASSIS_MODE_FOLLOW_GIMBAL: Follow gimbal by follow encoder.
	CHASSIS_MODE_INDENPENDENT: Run independently.
	CHASSIS_MODE_OPEN: Direct apply force without pid control.
*/
typedef enum {
	CHASSIS_MODE_RELAX,
	CHASSIS_MODE_BREAK,
	CHASSIS_MODE_FOLLOW_GIMBAL,
	CHASSIS_MODE_INDENPENDENT,
	CHASSIS_MODE_OPEN,
} Chassis_Mode_t;

typedef struct {
	float vx;
	float vy;
	float wz;
}  Chassis_ControlVector_t;

typedef struct {
	Chassis_Mode_t mode;
	Chassis_Mode_t last_mode;
	
	Chassis_ControlVector_t vector_set;
	Chassis_ControlVector_t vector_get;
	
	int power_limit;
	int power_consumpetion;
	PID_t motor_pid[4];
	PID_t follow_pid;
} Chassis_t;

/* Exported functions prototypes ---------------------------------------------*/
void Chassis_Init(Chassis_t* chassis);
void Chassis_SetMode(Chassis_t* chassis);
void Chassis_Control(Chassis_t* chassis);
void Chassis_SetOutput(Chassis_t* chassis);
