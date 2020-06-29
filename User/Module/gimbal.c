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
int Gimbal_SetMode(Gimbal_t *gimb, Gimbal_Mode_t mode) {
	if (gimb == NULL)
		return -1;
	
	if (mode == gimb->mode)
		return GIMBAL_OK;
	
	gimb->mode = mode;
	
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
int Gimbal_Init(Gimbal_t *gimb, const Gimbal_Params_t *gimb_param) {
	if (gimb == NULL)
		return -1;
	
	gimb->mode = GIMBAL_MODE_INIT;

	PID_Init(&(gimb->yaw_inner_pid), PID_MODE_DERIVATIV_SET, gimb->dt_sec, &(gimb_param->yaw_inner_pid_param));
	PID_Init(&(gimb->yaw_outer_pid), PID_MODE_DERIVATIV_NONE, gimb->dt_sec, &(gimb_param->yaw_outer_pid_param));
	PID_Init(&(gimb->pit_inner_pid), PID_MODE_DERIVATIV_SET, gimb->dt_sec, &(gimb_param->pit_inner_pid_param));
	PID_Init(&(gimb->pit_outer_pid), PID_MODE_DERIVATIV_NONE, gimb->dt_sec, &(gimb_param->pit_outer_pid_param));
	
	LowPassFilter2p_Init(&(gimb->yaw_output_filter), 1000.f / gimb->dt_sec, 100.f);
	LowPassFilter2p_Init(&(gimb->pit_output_filter), 1000.f / gimb->dt_sec, 100.f);
	return 0;
}


int Gimbal_UpdateFeedback(Gimbal_t *gimb, CAN_Device_t *can_device) {
	if (gimb == NULL)
		return -1;
	
	if (can_device == NULL)
		return -1;
	
	const float yaw_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	gimb->encoder_eulr.yaw = yaw_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	const float pit_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	gimb->encoder_eulr.pit = pit_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	return 0;
}

int Gimbal_ParseCommand(Gimbal_Ctrl_t *gimb_ctrl, const DR16_t *dr16) {
	if (gimb_ctrl == NULL)
		return -1;
	
	if (dr16 == NULL)
		return -1;
	
		/* RC Control. */
	switch (dr16->data.rc.sw_l) {
		case DR16_SW_UP:
		case DR16_SW_MID:
		case DR16_SW_DOWN:
			gimb_ctrl->mode = GIMBAL_MODE_ABSOLUTE;
			break;
		
		case DR16_SW_ERR:
			gimb_ctrl->mode = GIMBAL_MODE_RELAX;
			break;
	}
	
	gimb_ctrl->ctrl_eulr.yaw += dr16->data.rc.ch_r_x;
	gimb_ctrl->ctrl_eulr.pit += dr16->data.rc.ch_r_y;
	
	if ((dr16->data.rc.sw_l == DR16_SW_UP) && (dr16->data.rc.sw_r == DR16_SW_UP)) {
		/* PC Control. */
		gimb_ctrl->ctrl_eulr.yaw += (float)dr16->data.mouse.x / 100.f;	
		gimb_ctrl->ctrl_eulr.pit += (float)dr16->data.mouse.y / 100.f;
		
	}
	return 0;
}

int Gimbal_Control(Gimbal_t *gimb, Gimbal_Ctrl_t *gimb_ctrl) {
	if (gimb == NULL)
		return -1;
	
	if (gimb_ctrl == NULL)
		return -1;
	
	if (gimb->imu == NULL)
		return -1;
	
	Gimbal_SetMode(gimb, gimb_ctrl->mode);
	
	float motor_gyro_set;
	
	switch(gimb->mode) {
		case GIMBAL_MODE_RELAX:
			gimb->pit_cur_out = 0;
			gimb->yaw_cur_out = 0;
			break;
		
		case GIMBAL_MODE_INIT:
		case GIMBAL_MODE_CALI:
			break;
		
		case GIMBAL_MODE_ABSOLUTE:
			motor_gyro_set = PID_Calc(&(gimb->yaw_inner_pid), gimb_ctrl->ctrl_eulr.yaw, gimb->imu_eulr->yaw, gimb->imu->gyro.z, gimb->dt_sec);
			gimb->yaw_cur_out  = PID_Calc(&(gimb->yaw_outer_pid), motor_gyro_set, gimb->imu->gyro.z, 0.f, gimb->dt_sec);
			
			motor_gyro_set = PID_Calc(&(gimb->pit_inner_pid), gimb_ctrl->ctrl_eulr.pit, gimb->imu_eulr->pit, gimb->imu->gyro.x, gimb->dt_sec);
			gimb->pit_cur_out  = PID_Calc(&(gimb->pit_outer_pid), motor_gyro_set, gimb->imu->gyro.x, 0.f, gimb->dt_sec);
			break;
			
		case GIMBAL_MODE_FIX:
			gimb_ctrl->ctrl_eulr.yaw = 0.f;
			gimb_ctrl->ctrl_eulr.pit = 0.f;
			/* NO break. */
		
		case GIMBAL_MODE_RELATIVE:
			motor_gyro_set = PID_Calc(&(gimb->yaw_inner_pid), gimb_ctrl->ctrl_eulr.yaw, gimb->encoder_eulr.yaw, gimb->imu->gyro.z, gimb->dt_sec);
			gimb->yaw_cur_out  = PID_Calc(&(gimb->yaw_outer_pid), motor_gyro_set, gimb->imu->gyro.z, 0.f, gimb->dt_sec);
			
			motor_gyro_set = PID_Calc(&(gimb->pit_inner_pid), gimb_ctrl->ctrl_eulr.pit, gimb->encoder_eulr.pit, gimb->imu->gyro.x, gimb->dt_sec);
			gimb->pit_cur_out  = PID_Calc(&(gimb->pit_outer_pid), motor_gyro_set, gimb->imu->gyro.x, 0.f, gimb->dt_sec);
			break;
			
		default:
			return -1;
	}
	
	/* Filter output. */
	gimb->yaw_cur_out = LowPassFilter2p_Apply(&(gimb->yaw_output_filter), gimb->yaw_cur_out);
	gimb->pit_cur_out = LowPassFilter2p_Apply(&(gimb->pit_output_filter), gimb->pit_cur_out);
	
	
	return 0;
}
