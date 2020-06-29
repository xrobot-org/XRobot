#pragma once


/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
#include "can_device.h"
#include "dr16.h"

/* Include Component相关的头文件 */
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
/*  
	SHOOT_MODE_RELAX: No force applied.
	SHOOT_MODE_SAFE: Set to zero speed. Force applied.
	SHOOT_MODE_STDBY: Ready to switch to FIRE.
	SHOOT_MODE_FIRE: Ready to shoot.
*/

typedef enum {
	SHOOT_MODE_RELAX,
	SHOOT_MODE_SAFE,
	SHOOT_MODE_STDBY,
	SHOOT_MODE_FIRE,
} Shoot_Mode_t;

typedef struct {
	float bullet_speed;
	float shoot_freq_hz;
	Shoot_Mode_t mode;
} Shoot_Ctrl_t;

typedef struct {
	PID_Params_t fric_pid_param[2];
	PID_Params_t trig_pid_param;
	float low_pass_cutoff;
} Shoot_Params_t;

typedef struct {
	const Shoot_Params_t *params;
	
	/* common */
	float dt_sec;
	Shoot_Mode_t mode;
	osTimerId_t trig_timer_id;
	
	/* Feedback */
	float fric_rpm[2];
	float trig_angle;
	
	/* PID set point */
	float fric_rpm_set[2];
	float trig_angle_set;
	
	/* PID */
	PID_t fric_pid[2];
	PID_t trig_pid;
	
	/* Output */
	float fric_cur_out[2];
	float trig_cur_out;
	
	int heat_limiter;
	
	/* Output filter */
	LowPassFilter2p_t fric_output_filter[2];
	LowPassFilter2p_t trig_output_filter;

} Shoot_t;


/* Exported functions prototypes ---------------------------------------------*/
int Shoot_Init(Shoot_t *shoot, const Shoot_Params_t *shoot_param);
int Shoot_UpdateFeedback(Shoot_t *shoot, CAN_Device_t *can_device);
int Shoot_ParseCommand(Shoot_Ctrl_t *shoot_ctrl, const DR16_t *dr16);
int Shoot_Control(Shoot_t *shoot, Shoot_Ctrl_t *shoot_ctrl);
