#pragma once


/* Includes ------------------------------------------------------------------*/
/* Include 标准库 */
/* Include Board相关的头文件  */
/* Include Device相关的头文件。 */
#include "can_device.h"
#include "dr16.h"
#include "bmi088.h"

/* Include Component相关的头文件。 */
#include "pid.h"
#include "ahrs.h"
#include "filter.h"

/* Include Module相关的头文件。 */
/* Exported constants --------------------------------------------------------*/\
#define GIMBAL_OK		(0)
#define GIMBAL_ERR		(-1)
#define GIMBAL_ERR_MODE	(-2)

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
	GIMBAL_MODE_ABSOLUTE,
	GIMBAL_MODE_RELATIVE,
	GIMBAL_MODE_FIX,
} Gimbal_Mode_t;

typedef struct {
	AHRS_Eulr_t ctrl_eulr;
	Gimbal_Mode_t mode;
} Gimbal_Ctrl_t;

typedef struct {
	/* common */
	float dt_sec;
	Gimbal_Mode_t mode;
	
	/* Feedback */
	BMI088_t *imu;
	AHRS_Eulr_t *imu_eulr;
	AHRS_Eulr_t encoder_eulr;
	
	/* PID */
	PID_t yaw_inner_pid;
	PID_t yaw_outer_pid;
	
	PID_t pit_inner_pid;
	PID_t pit_outer_pid;
	
	/* Output */
	float yaw_cur_out;
	float pit_cur_out;
	
	/* Output filter */
	LowPassFilter2p_t yaw_output_filter;
	LowPassFilter2p_t pit_output_filter;
} Gimbal_t;

typedef struct {
	PID_Params_t yaw_inner_pid_param;
	PID_Params_t yaw_outer_pid_param;
	
	PID_Params_t pit_inner_pid_param;
	PID_Params_t pit_outer_pid_param;
	
	float low_pass_cutoff;
} Gimbal_Params_t;

/* Exported functions prototypes ---------------------------------------------*/
int Gimbal_Init(Gimbal_t *gimb, const Gimbal_Params_t *gimb_param);
int Gimbal_SetMode(Gimbal_t *gimb, Gimbal_Mode_t mode);
int Gimbal_UpdateFeedback(Gimbal_t *gimb, CAN_Device_t *can_device);
int Gimbal_ParseCommand(Gimbal_t *gimb, Gimbal_Ctrl_t *gimb_ctrl, const DR16_t *dr16);
int Gimbal_Control(Gimbal_t *gimb, AHRS_Eulr_t *ctrl_eulr);
