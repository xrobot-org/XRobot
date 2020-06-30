/* 
	底盘模组

*/


/* Includes ------------------------------------------------------------------*/
#include "gimbal.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
#include "user_math.h"

/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
int Gimbal_SetMode(Gimbal_t *g, Gimbal_Mode_t mode) {
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
int Gimbal_Init(Gimbal_t *g, const Gimbal_Params_t *g_param) {
	if (g == NULL)
		return -1;
	
	g->mode = GIMBAL_MODE_INIT;

	PID_Init(&(g->pid[GIMBAL_PID_PIT_IN]), PID_MODE_SET_D, g->dt_sec, &(g_param->pid[GIMBAL_PID_PIT_IN]));
	PID_Init(&(g->pid[GIMBAL_PID_YAW_OUT]), PID_MODE_NO_D, g->dt_sec, &(g_param->pid[GIMBAL_PID_YAW_OUT]));
	PID_Init(&(g->pid[GIMBAL_PID_PIT_IN]), PID_MODE_SET_D, g->dt_sec, &(g_param->pid[GIMBAL_PID_PIT_IN]));
	PID_Init(&(g->pid[GIMBAL_PID_PIT_OUT]), PID_MODE_NO_D, g->dt_sec, &(g_param->pid[GIMBAL_PID_PIT_OUT]));
	
	LowPassFilter2p_Init(&(g->filter[GIMBAL_LPF_YAW]), 1000.f / g->dt_sec, 100.f);
	LowPassFilter2p_Init(&(g->filter[GIMBAL_LPF_PIT]), 1000.f / g->dt_sec, 100.f);
	return 0;
}


int Gimbal_UpdateFeedback(Gimbal_t *g, CAN_Device_t *can_device) {
	if (g == NULL)
		return -1;
	
	if (can_device == NULL)
		return -1;
	
	const float yaw_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	g->encoder_eulr.yaw = yaw_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	const float pit_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	g->encoder_eulr.pit = pit_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	return 0;
}

int Gimbal_ParseCommand(Gimbal_Ctrl_t *g_ctrl, const DR16_t *dr16) {
	if (g_ctrl == NULL)
		return -1;
	
	if (dr16 == NULL)
		return -1;
	
		/* RC Control. */
	switch (dr16->data.rc.sw_l) {
		case DR16_SW_UP:
		case DR16_SW_MID:
		case DR16_SW_DOWN:
			g_ctrl->mode = GIMBAL_MODE_ABSOLUTE;
			break;
		
		case DR16_SW_ERR:
			g_ctrl->mode = GIMBAL_MODE_RELAX;
			break;
	}
	
	g_ctrl->eulr.yaw += dr16->data.rc.ch_r_x;
	g_ctrl->eulr.pit += dr16->data.rc.ch_r_y;
	
	if ((dr16->data.rc.sw_l == DR16_SW_UP) && (dr16->data.rc.sw_r == DR16_SW_UP)) {
		/* PC Control. */
		g_ctrl->eulr.yaw += (float)dr16->data.mouse.x / 100.f;	
		g_ctrl->eulr.pit += (float)dr16->data.mouse.y / 100.f;
		
	}
	return 0;
}

int Gimbal_Control(Gimbal_t *g, Gimbal_Ctrl_t *g_ctrl) {
	if (g == NULL)
		return -1;
	
	if (g_ctrl == NULL)
		return -1;
	
	if (g->imu == NULL)
		return -1;
	
	Gimbal_SetMode(g, g_ctrl->mode);
	
	float motor_gyro_set;
	
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
