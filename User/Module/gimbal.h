#pragma once


/* Includes ------------------------------------------------------------------*/
/* Include 标准库 */
/* Include Board相关的头文件  */
/* Include Device相关的头文件。 */
#include "can_device.h"

/* Include Component相关的头文件。 */
#include "pid.h"
#include "ahrs.h"
#include "imu.h"

/* Include Module相关的头文件。 */
/* Exported constants --------------------------------------------------------*/\
#define GIMBAL_OK		(0)
#define GIMBAL_ERR		(-1)
#define GIMBAL_ERR_MODE	(-2)


/* PID paramters. */
#define GIMBAL_ABSOLUTE_YAW_INNER_PID_KP        26.0f
#define GIMBAL_ABSOLUTE_YAW_INNER_PID_KI        0.0f
#define GIMBAL_ABSOLUTE_YAW_INNER_PID_KD        0.3f
#define GIMBAL_ABSOLUTE_YAW_INNER_PID_MAX_OUT   10.0f
#define GIMBAL_ABSOLUTE_YAW_INNER_PID_MAX_IOUT  0.0f

#define GIMBAL_ABSOLUTE_PIT_INNER_PID_KP        15.0f
#define GIMBAL_ABSOLUTE_PIT_INNER_PID_KI        0.0f
#define GIMBAL_ABSOLUTE_PIT_INNER_PID_KD        0.0f
#define GIMBAL_ABSOLUTE_PIT_INNER_PID_MAX_OUT   10.0f
#define GIMBAL_ABSOLUTE_PIT_INNER_PID_MAX_IOUT  0.0f

#define GIMBAL_RELATIVE_YAW_INNER_PID_KP        26.0f
#define GIMBAL_RELATIVE_YAW_INNER_PID_KI        0.0f
#define GIMBAL_RELATIVE_YAW_INNER_PID_KD        0.3f
#define GIMBAL_RELATIVE_YAW_INNER_PID_MAX_OUT   10.0f
#define GIMBAL_RELATIVE_YAW_INNER_PID_MAX_IOUT  0.0f

#define GIMBAL_RELATIVE_PIT_INNER_PID_KP        15.0f
#define GIMBAL_RELATIVE_PIT_INNER_PID_KI        0.0f
#define GIMBAL_RELATIVE_PIT_INNER_PID_KD        0.0f
#define GIMBAL_RELATIVE_PIT_INNER_PID_MAX_OUT   10.0f
#define GIMBAL_RELATIVE_PIT_INNER_PID_MAX_IOUT  0.0f

#define GIMBAL_YAW_OUTER_PID_KP        3600.0f
#define GIMBAL_YAW_OUTER_PID_KI        20.0f
#define GIMBAL_YAW_OUTER_PID_KD        0.0f
#define GIMBAL_YAW_OUTER_PID_MAX_OUT   30000.0f
#define GIMBAL_YAW_OUTER_PID_MAX_IOUT  5000.0f

#define GIMBAL_PIT_OUTER_PID_KP        2900.0f
#define GIMBAL_PIT_OUTER_PID_KI        60.0f
#define GIMBAL_PIT_OUTER_PID_KD        0.0f
#define GIMBAL_PIT_OUTER_PID_MAX_OUT   30000.0f
#define GIMBAL_PIT_OUTER_PID_MAX_IOUT  10000.0f

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/*  
	GIMBAL_MODE_RELAX: No force applied.
	GIMBAL_MODE_INIT: .
	GIMBAL_MODE_CALI: Get mid point.
	GIMBAL_MODE_ABSOLUTE: Follow IMU data.
	GIMBAL_MODE_RELATIVE: Follow encoder data.
	GIMBAL_MODE_FIX: Set to a fix angle. Force applied.
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
	/* common */
	float dt_ms;
	
	Gimbal_Mode_t mode;
	Gimbal_Mode_t last_mode;
	
	/* Feedback */
	float yaw_encoder_angle;
	float pit_encoder_angle;
	IMU_t *imu;
	AHRS_Eulr_t *gimb_eulr;
	
	/* Input */
	AHRS_Eulr_t *ctrl_eulr;
	
	/* PID set point */
	float motor_pos_set[4];
	
	/* PID */
	PID_t yaw_inner_pid;
	PID_t yaw_outer_pid;
	
	PID_t pit_inner_pid;
	PID_t pit_outer_pid;
	
	/* Output */
	float pit_cur_out;
	float yaw_cur_out;
	
} Gimbal_t;

/* Exported functions prototypes ---------------------------------------------*/
int Gimbal_Init(Gimbal_t *gimb);
int Gimbal_SetMode(Gimbal_t *gimb, Gimbal_Mode_t mode);
int Gimbal_UpdateFeedback(Gimbal_t *gimb, CAN_Device_t *can_device);
int Gimbal_Control(Gimbal_t *gimb);
