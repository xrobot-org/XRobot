#pragma once


/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
#include "can_device.h"
#include "dr16.h"

/* Include Component相关的头文件 */
#include "cmd.h"
#include "pid.h"
#include "filter.h"

/* Include Module相关的头文件 */
/* Exported constants --------------------------------------------------------*/
#define SHOOT_OK		(0)
#define SHOOT_ERR		(-1)
#define SHOOT_ERR_MODE	(-2)

#define SHOOT_BULLET_SPEED_SCALER (2.f)
#define SHOOT_BULLET_SPEED_BIAS  (1.f)

#define SHOOT_NUM_FEEDING_TOOTH  (8u)

/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	PID_Params_t fric_pid_param[2];
	PID_Params_t trig_pid_param;
	float32_t low_pass_cutoff;
} Shoot_Params_t;

typedef struct {
	const Shoot_Params_t *params;
	
	/* common */
	float32_t dt_sec;
	CMD_Shoot_Mode_t mode;
	osTimerId_t trig_timer_id;
	
	/* Feedback */
	float32_t fric_rpm[2];
	float32_t trig_angle;
	
	/* PID set point */
	float32_t fric_rpm_set[2];
	float32_t trig_angle_set;
	
	/* PID */
	PID_t fric_pid[2];
	PID_t trig_pid;
	
	/* Output */
	float32_t fric_cur_out[2];
	float32_t trig_cur_out;
	
	int8_t heat_limiter;
	
	/* Output filter */
	LowPassFilter2p_t fric_output_filter[2];
	LowPassFilter2p_t trig_output_filter;

} Shoot_t;


/* Exported functions prototypes ---------------------------------------------*/
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *param, float32_t dt_sec);
int8_t Shoot_UpdateFeedback(Shoot_t *s, CAN_Device_t *can_device);
int8_t Shoot_Control(Shoot_t *s, CMD_Shoot_Ctrl_t *s_ctrl);
