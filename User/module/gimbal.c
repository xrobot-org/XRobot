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
	
	bool con1 = g->fb.imu->gyro.x < 0.1f;
	bool con2 = g->fb.imu->gyro.y < 0.1f;
	bool con3 = g->fb.imu->gyro.z < 0.1f;
	
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
int8_t Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *param, float dt_sec, BMI088_t *imu){
	if (g == NULL)
		return -1;
	
	g->mode = GIMBAL_MODE_INIT;
	g->dt_sec = dt_sec;
	g->fb.imu = imu;

	PID_Init(&(g->pid[GIMBAL_PID_PIT_IN]), PID_MODE_SET_D, g->dt_sec, &(param->pid[GIMBAL_PID_PIT_IN]));
	PID_Init(&(g->pid[GIMBAL_PID_YAW_OUT]), PID_MODE_NO_D, g->dt_sec, &(param->pid[GIMBAL_PID_YAW_OUT]));
	PID_Init(&(g->pid[GIMBAL_PID_PIT_IN]), PID_MODE_SET_D, g->dt_sec, &(param->pid[GIMBAL_PID_PIT_IN]));
	PID_Init(&(g->pid[GIMBAL_PID_PIT_OUT]), PID_MODE_NO_D, g->dt_sec, &(param->pid[GIMBAL_PID_PIT_OUT]));
	
	for (uint8_t i  = 0; i < GIMBAL_ACTR_NUM; i++)
		LowPassFilter2p_Init(&(g->filter[i]), 1.f / g->dt_sec, param->low_pass_cutoff);
	
	return 0;
}


int8_t Gimbal_UpdateFeedback(Gimbal_t *g, CAN_t *can) {
	if (g == NULL)
		return -1;
	
	if (can == NULL)
		return -1;
	
	const float yaw_angle = can->gimbal_motor_fb.yaw_fb.rotor_angle;
	g->eulr.encoder.yaw = yaw_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * M_PI;
	
	const float pit_angle = can->gimbal_motor_fb.yaw_fb.rotor_angle;
	g->eulr.encoder.pit = pit_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * M_PI;
	
	return 0;
}

int8_t Gimbal_Control(Gimbal_t *g, CMD_Gimbal_Ctrl_t *g_ctrl) {
	if (g == NULL)
		return -1;
	
	if (g_ctrl == NULL)
		return -1;
	
	if (g->fb.imu == NULL)
		return -1;
	
	Gimbal_SetMode(g, g_ctrl->mode);
	
	g->set.eulr.yaw += g_ctrl->delta_eulr.yaw;
	g->set.eulr.pit += g_ctrl->delta_eulr.pit;
	
	float motor_gyro_set;
	switch(g->mode) {
		case GIMBAL_MODE_RELAX:
			for (uint8_t i  = 0; i < GIMBAL_ACTR_NUM; i++)
				g->out[i] = 0.f;
			break;
		
		case GIMBAL_MODE_ABSOLUTE:
			motor_gyro_set = PID_Calc(&(g->pid[GIMBAL_PID_YAW_IN]), g->set.eulr.yaw, g->fb.eulr.imu->yaw, g->fb.imu->gyro.z, g->dt_sec);
			g->out[GIMBAL_ACTR_YAW] = PID_Calc(&(g->pid[GIMBAL_PID_YAW_OUT]), motor_gyro_set, g->fb.imu->gyro.z, 0.f, g->dt_sec);
			
			motor_gyro_set = PID_Calc(&(g->pid[GIMBAL_PID_PIT_IN]), g->set.eulr.pit, g->fb.eulr.imu->pit, g->fb.imu->gyro.x, g->dt_sec);
			g->out[GIMBAL_ACTR_PIT] = PID_Calc(&(g->pid[GIMBAL_PID_PIT_OUT]), motor_gyro_set, g->fb.imu->gyro.x, 0.f, g->dt_sec);
			break;
		
		case GIMBAL_MODE_INIT:
			if (Gimbal_Steady(g))
				g->mode = GIMBAL_MODE_RELAX;
			
		case GIMBAL_MODE_FIX:
			g->set.eulr.pit = 0.f;
			g->set.eulr.pit = 0.f;
			/* NO break. */
		
		case GIMBAL_MODE_RELATIVE:
			g->out[GIMBAL_ACTR_YAW] = PID_Calc(&(g->pid[GIMBAL_PID_REL_YAW]), g->set.eulr.yaw, g->fb.eulr.encoder.yaw, g->fb.imu->gyro.z, g->dt_sec);
			g->out[GIMBAL_ACTR_PIT] = PID_Calc(&(g->pid[GIMBAL_PID_REL_PIT]), g->set.eulr.pit, g->fb.eulr.encoder.pit, g->fb.imu->gyro.x, g->dt_sec);
			break;
	}
	/* Filter output. */
	for (uint8_t i  = 0; i < GIMBAL_ACTR_NUM; i++)
		g->out[i] = LowPassFilter2p_Apply(&(g->filter[i]), g->out[i]);
	
	return 0;
}
