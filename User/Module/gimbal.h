#pragma once


/* Includes ------------------------------------------------------------------*/
/* Include 标准库*/
/* Include Board相关的头文件 */
/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
#include "pid.h"
#include "ahrs.h"

/* Include Module相关的头文件。*/
/* Exported constants --------------------------------------------------------*/
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
	GIMBAL_MODE_RELAX,
	GIMBAL_MODE_INIT,
	GIMBAL_MODE_CALI,
	GIMBAL_MODE_ABSOLUTE, /* Use IMU */
	GIMBAL_MODE_RELATIVE, /* Use encoder */
	GIMBAL_MODE_FIX, /* Use encoder */
} Gimbal_Mode_t;

typedef struct {
	float motor;
	int mode ;
	int mixer;
	
} Gimbal_t;


/* Exported functions prototypes ---------------------------------------------*/
void Gimbal_Init(Gimbal_t *gimbal);
void Gimbal_SetMode(Gimbal_t *gimbal);
void Gimbal_Control(Gimbal_t *gimbal);
void Gimbal_SetOutput(Gimbal_t *gimbal);
