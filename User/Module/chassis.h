#pragma once


/* Includes ------------------------------------------------------------------*/
/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
#include "can_device.h"
#include "dr16.h"

/* Include Component相关的头文件 */
#include "cmd.h"
#include "filter.h"
#include "mixer.h"
#include "pid.h"
#include "user_math.h"


/* Include Module相关的头文件 */
/* Exported constants --------------------------------------------------------*/
#define CHASSIS_OK			(0)
#define CHASSIS_ERR_NULL	(-1)
#define CHASSIS_ERR_MODE	(-2)
#define CHASSIS_ERR_TYPE	(-3)

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

typedef struct {
	Chassis_Type_t type;
	PID_Params_t *motor_pid_param;
	PID_Params_t follow_pid_param;
	float32_t low_pass_cutoff;
} Chassis_Params_t;

typedef struct {
	const Chassis_Params_t *params;
	
	/* common */
	float32_t dt_sec;
	CMD_Chassis_Mode_t mode;
	
	/* Chassis design */
	int8_t num_wheel;
	Mixer_t mixer;
	
	/* Feedback */
	float32_t gimbal_yaw_angle;
	float32_t *motor_rpm;
	
	/* Mid product */
	MoveVector_t move_vec;
	
	/* Mixer Out / PID set point. */
	float32_t *motor_rpm_set;
	float32_t motor_scaler;
	
	/* PID */
	PID_t *motor_pid;
	PID_t follow_pid;
	
	/* Output */
	float32_t *motor_cur_out;
	
	/* Output filter */
	LowPassFilter2p_t *output_filter;
} Chassis_t;

/* Exported functions prototypes ---------------------------------------------*/
int8_t Chassis_Init(Chassis_t *c, const Chassis_Params_t *param, float32_t dt_sec);
int8_t Chassis_UpdateFeedback(Chassis_t *c, CAN_Device_t *can_device);
int8_t Chassis_Control(Chassis_t *c, CMD_Chassis_Ctrl_t *c_ctrl);
