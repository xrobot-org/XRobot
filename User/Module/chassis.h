#pragma once


/* Includes ------------------------------------------------------------------*/
/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
#include "can_device.h"

/* Include Component相关的头文件 */
#include "pid.h"
#include "mixer.h"

/* Include Module相关的头文件 */
/* Exported constants --------------------------------------------------------*/
#define CHASSIS_OK			(0)
#define CHASSIS_ERR			(-1)
#define CHASSIS_ERR_MODE	(-2)
#define CHASSIS_ERR_type	(-3)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/*  
	CHASSIS_TYPE_MECANUM: For infantry, hero and engineer.
	CHASSIS_MODE_PARLFIX4: For sentry.
	CHASSIS_MODE_PARLFIX2: For sentry.
	CHASSIS_MODE_OMNI_CROSS: Not implemented. Saved for future.
	CHASSIS_MODE_OMNI_PLUS: Not implemented. Saved for future.
	CHASSIS_MODE_DRONE: For drone.
*/
typedef enum {
	CHASSIS_TYPE_MECANUM,
	CHASSIS_MODE_PARLFIX4,
	CHASSIS_MODE_PARLFIX2,
	CHASSIS_MODE_OMNI_CROSS,
	CHASSIS_MODE_OMNI_PLUS,
	CHASSIS_MODE_DRONE,
} Chassis_Type_t;

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

typedef struct {
	float vx;
	float vy;
	float wz;
}  CtrlVector_t;

typedef struct {
	/* common */
	float control_time;
	
	Chassis_Mode_t mode;
	Chassis_Mode_t last_mode;
	int wheel_num;
	
	Chassis_Type_t type;
	
	int (*Mix)(float vx, float vy, float vz, float *out, int len);
	
	/* Input */
	float gimbal_yaw_angle;
	float motor_speed[4];
	CtrlVector_t *robot_ctrl_v;
	
	/* Set point */
	CtrlVector_t chas_ctrl_v;
	
	/* PID */
	PID_t wheel_pid[4];
	PID_t follow_pid;
	
	/* Mixer Out / PID set point. */
	float motor_rpm_set[4];
	
	/* Chassis Out */
	float motor_cur_out[4];
	
	int power_limit;
	int power_consumpetion;
	
} Chassis_t;

/* Exported functions prototypes ---------------------------------------------*/
int Chassis_Init(Chassis_t *chas, Chassis_Type_t type);
int Chassis_SetMode(Chassis_t *chas, Chassis_Mode_t mode);
int Chassis_UpdateFeedback(Chassis_t *chas, CAN_Device_t *can_device);
int Chassis_Control(Chassis_t *chas);
