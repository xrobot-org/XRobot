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
#include "user_math.h"

/* Include Module相关的头文件。 */
/* Exported constants --------------------------------------------------------*/
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

enum Gimbal_PID_e{
	GIMBAL_PID_YAW_IN = 0,
	GIMBAL_PID_YAW_OUT,
	GIMBAL_PID_PIT_IN,
	GIMBAL_PID_PIT_OUT,
	GIMBAL_PID_NUM,
};

enum Gimbal_LPF_e{
	GIMBAL_LPF_YAW = 0,
	GIMBAL_LPF_PIT,
	GIMBAL_LPF_NUM,
};

typedef struct {
	AHRS_Eulr_t eulr;
	Gimbal_Mode_t mode;
} Gimbal_Ctrl_t;

typedef struct {
	PID_Params_t pid[GIMBAL_PID_NUM];
	float32_t low_pass_cutoff;
} Gimbal_Params_t;

typedef struct {
	const Gimbal_Params_t *params;
	
	/* common */
	float32_t dt_sec;
	Gimbal_Mode_t mode;
	
	/* Feedback */
	BMI088_t *imu;
	AHRS_Eulr_t *imu_eulr;
	AHRS_Eulr_t encoder_eulr;
	
	/* PID */
	PID_t pid[GIMBAL_PID_NUM];
	
	/* Output */
	float32_t yaw_cur_out;
	float32_t pit_cur_out;
	
	/* Output filter */
	LowPassFilter2p_t filter[GIMBAL_LPF_NUM];
} Gimbal_t;


/* Exported functions prototypes ---------------------------------------------*/
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *g_param);
int8_t Gimbal_UpdateFeedback(Gimbal_t *g, CAN_Device_t *can_device);
int8_t Gimbal_ParseCommand(Gimbal_Ctrl_t *g_ctrl, const DR16_t *dr16);
int8_t Gimbal_Control(Gimbal_t *g, Gimbal_Ctrl_t *g_ctrl);
