/* 
	底盘模组

*/


/* Includes ------------------------------------------------------------------*/
#include "gimbal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
static int8_t Gimbal_SetMode(Gimbal_t *g, CMD_Gimbal_Mode_t mode) {
	if (g == NULL)
		return -1;
	
	if (mode == g->mode)
		return GIMBAL_OK;
	
	if (g->mode != GIMBAL_MODE_INIT)
		g->mode = mode;
	
	return 0;
}

static bool Gimbal_Steady(Gimbal_t *g) {
	static uint8_t steady;
	
	bool con1 = g->imu->gyro.x < 0.1;
	bool con2 = g->imu->gyro.y < 0.1;
	bool con3 = g->imu->gyro.z < 0.1;
	
	if (con1 && con2 && con3)	
		steady++;
	else {
		return false;
	}
	if (steady > 20) {
		steady = 0;
		return true;
	}
	return false;
}

/* Exported functions --------------------------------------------------------*/
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param, float32_t dt_sec, BMI088_t *imu){
	if (g == NULL)
		return -1;
	
	g->mode = GIMBAL_MODE_INIT;
	g->dt_sec = dt_sec;
	g->imu = imu;

	PID_Init(&(g->pid[GIMBAL_PID_PIT_IN]), PID_MODE_SET_D, g->dt_sec, &(param->pid[GIMBAL_PID_PIT_IN]));
	PID_Init(&(g->pid[GIMBAL_PID_YAW_OUT]), PID_MODE_NO_D, g->dt_sec, &(param->pid[GIMBAL_PID_YAW_OUT]));
	PID_Init(&(g->pid[GIMBAL_PID_PIT_IN]), PID_MODE_SET_D, g->dt_sec, &(param->pid[GIMBAL_PID_PIT_IN]));
	PID_Init(&(g->pid[GIMBAL_PID_PIT_OUT]), PID_MODE_NO_D, g->dt_sec, &(param->pid[GIMBAL_PID_PIT_OUT]));
	
	for (uint8_t i  = 0; i < GIMBAL_ACTR_NUM; i++)
		LowPassFilter2p_Init(&(g->filter[i]), 1.f / g->dt_sec, 100.f);
	
	return 0;
}


int8_t Gimbal_UpdateFeedback(Gimbal_t *g, CAN_Device_t *can_device) {
	if (g == NULL)
		return -1;
	
	if (can_device == NULL)
		return -1;
	
	const float32_t yaw_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	g->eulr.encoder.yaw = yaw_angle / (float32_t)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	const float32_t pit_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	g->eulr.encoder.pit = pit_angle / (float32_t)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	return 0;
}

int8_t Gimbal_Control(Gimbal_t *g, CMD_Gimbal_Ctrl_t *g_ctrl) {
	if (g == NULL)
		return -1;
	
	if (g_ctrl == NULL)
		return -1;
	
	if (g->imu == NULL)
		return -1;
	
	Gimbal_SetMode(g, g_ctrl->mode);
	
	float32_t motor_gyro_set;
	
	switch(g->mode) {
		case GIMBAL_MODE_RELAX:
			for (uint8_t i  = 0; i < GIMBAL_ACTR_NUM; i++)
				g->cur_out[i] = 0.f;
			break;
		
		case GIMBAL_MODE_ABSOLUTE:
			motor_gyro_set = PID_Calc(&(g->pid[GIMBAL_PID_YAW_IN]), g_ctrl->eulr.yaw, g->eulr.imu->yaw, g->imu->gyro.z, g->dt_sec);
			g->cur_out[GIMBAL_ACTR_YAW] = PID_Calc(&(g->pid[GIMBAL_PID_YAW_OUT]), motor_gyro_set, g->imu->gyro.z, 0.f, g->dt_sec);
			
			motor_gyro_set = PID_Calc(&(g->pid[GIMBAL_PID_PIT_IN]), g_ctrl->eulr.pit, g->eulr.imu->pit, g->imu->gyro.x, g->dt_sec);
			g->cur_out[GIMBAL_ACTR_PIT] = PID_Calc(&(g->pid[GIMBAL_PID_PIT_OUT]), motor_gyro_set, g->imu->gyro.x, 0.f, g->dt_sec);
			break;
		
		
		case GIMBAL_MODE_INIT:
			if (Gimbal_Steady(g))
				g->mode = GIMBAL_MODE_RELAX;
			
		case GIMBAL_MODE_FIX:
			g_ctrl->eulr.yaw = 0.f;
			g_ctrl->eulr.pit = 0.f;
			/* NO break. */
		
		case GIMBAL_MODE_RELATIVE:
			g->cur_out[GIMBAL_ACTR_YAW] = PID_Calc(&(g->pid[GIMBAL_PID_REL_YAW]), g_ctrl->eulr.yaw, g->eulr.encoder.yaw, g->imu->gyro.z, g->dt_sec);
			g->cur_out[GIMBAL_ACTR_PIT] = PID_Calc(&(g->pid[GIMBAL_PID_REL_PIT]), g_ctrl->eulr.pit, g->eulr.encoder.pit, g->imu->gyro.x, g->dt_sec);
			break;
			
		default:
			return -1;
	}
	
	
	/* Filter output. */
	for (uint8_t i  = 0; i < GIMBAL_ACTR_NUM; i++)
		g->cur_out[i] = LowPassFilter2p_Apply(&(g->filter[i]), g->cur_out[i]);
	
	return 0;
}
