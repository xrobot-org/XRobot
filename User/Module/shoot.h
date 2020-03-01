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

/* Include Module相关的头文件 */
/* Exported constants --------------------------------------------------------*/
#define SHOOT_BULLET_SPEED_SCALER (2.f)
#define SHOOT_BULLET_SPEED_BIAS  (1.f)

#define SHOOT_FEEDING_TOOTH_NUM  (8u)

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
	uint32_t shoot_freq_hz;
	Shoot_Mode_t mode;
} Shoot_Ctrl_t;

typedef struct {
	/* common */
	float dt_sec;
	Shoot_Mode_t mode;
	
	/* Feedback */
	float fric_rpm[2];
	float trig_angle;
	osTimerId_t trig_timer_id;
	
	/* PID set point */
	float fric_rpm_set[2];
	float trig_pos_set;
	
	/* PID */
	PID_t fric_pid[2];
	PID_t trig_pid;
	
	/* Output */
	float fric_cur_out[2];
	float trig_cur_out;
	
	int heat_limiter;
	
} Shoot_t;


/* Exported functions prototypes ---------------------------------------------*/
int Shoot_Init(Shoot_t *shoot);
int Shoot_SetMode(Shoot_t *shoot, Shoot_Mode_t mode);
int Shoot_UpdateFeedback(Shoot_t *shoot, CAN_Device_t *can_device);
int Shoot_ParseCommand(Shoot_Ctrl_t *shoot_ctrl, const DR16_t *dr16);
int Shoot_Control(Shoot_t *shoot, float bullet_speed, uint32_t shoot_freq_hz);
