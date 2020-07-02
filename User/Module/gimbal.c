/* 
	底盘模组

*/


/* Includes ------------------------------------------------------------------*/
#include "gimbal.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
int8_t Gimbal_SetMode(Gimbal_t *g, CMD_Gimbal_Mode_t mode) {
	if (g == NULL)
		return -1;
	
	if (mode == g->mode)
		return GIMBAL_OK;
	
	g->mode = mode;
	
	// TODO: Check mode switchable.
	switch (mode) {
		case GIMBAL_MODE_RELAX:
			break;
		
		case GIMBAL_MODE_INIT:
			break;
		
		case GIMBAL_MODE_CALI:
			break;
		
		case GIMBAL_MODE_ABSOLUTE:
			break;
		
		case GIMBAL_MODE_RELATIVE:
			break;
		
		case GIMBAL_MODE_FIX:
			break;
		
		default:
			return -1;
	}
	return 0;
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
	
	LowPassFilter2p_Init(&(g->filter[GIMBAL_LPF_YAW]), 1000.f / g->dt_sec, 100.f);
	LowPassFilter2p_Init(&(g->filter[GIMBAL_LPF_PIT]), 1000.f / g->dt_sec, 100.f);
	return 0;
}


int8_t Gimbal_UpdateFeedback(Gimbal_t *g, CAN_Device_t *can_device) {
	if (g == NULL)
		return -1;
	
	if (can_device == NULL)
		return -1;
	
	const float32_t yaw_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	g->encoder_eulr.yaw = yaw_angle / (float32_t)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	const float32_t pit_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	g->encoder_eulr.pit = pit_angle / (float32_t)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
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
			g->pit_cur_out = 0;
			g->yaw_cur_out = 0;
			break;
		
		case GIMBAL_MODE_INIT:
		case GIMBAL_MODE_CALI:
			break;
		
		case GIMBAL_MODE_ABSOLUTE:
			motor_gyro_set = PID_Calc(&(g->pid[GIMBAL_PID_YAW_IN]), g_ctrl->eulr.yaw, g->imu_eulr->yaw, g->imu->gyro.z, g->dt_sec);
			g->yaw_cur_out = PID_Calc(&(g->pid[GIMBAL_PID_YAW_OUT]), motor_gyro_set, g->imu->gyro.z, 0.f, g->dt_sec);
			
			motor_gyro_set = PID_Calc(&(g->pid[GIMBAL_PID_PIT_IN]), g_ctrl->eulr.pit, g->imu_eulr->pit, g->imu->gyro.x, g->dt_sec);
			g->pit_cur_out = PID_Calc(&(g->pid[GIMBAL_PID_PIT_OUT]), motor_gyro_set, g->imu->gyro.x, 0.f, g->dt_sec);
			break;
			
		case GIMBAL_MODE_FIX:
			g_ctrl->eulr.yaw = 0.f;
			g_ctrl->eulr.pit = 0.f;
			/* NO break. */
		
		case GIMBAL_MODE_RELATIVE:
			motor_gyro_set = PID_Calc(&(g->pid[GIMBAL_PID_YAW_IN]), g_ctrl->eulr.yaw, g->encoder_eulr.yaw, g->imu->gyro.z, g->dt_sec);
			g->yaw_cur_out = PID_Calc(&(g->pid[GIMBAL_PID_YAW_OUT]), motor_gyro_set, g->imu->gyro.z, 0.f, g->dt_sec);
			
			motor_gyro_set = PID_Calc(&(g->pid[GIMBAL_PID_PIT_IN]), g_ctrl->eulr.pit, g->encoder_eulr.pit, g->imu->gyro.x, g->dt_sec);
			g->pit_cur_out = PID_Calc(&(g->pid[GIMBAL_PID_PIT_OUT]), motor_gyro_set, g->imu->gyro.x, 0.f, g->dt_sec);
			break;
			
		default:
			return -1;
	}
	
	/* Filter output. */
	g->yaw_cur_out = LowPassFilter2p_Apply(&(g->filter[GIMBAL_LPF_YAW]), g->yaw_cur_out);
	g->pit_cur_out = LowPassFilter2p_Apply(&(g->filter[GIMBAL_LPF_YAW]), g->pit_cur_out);
	
	return 0;
}
