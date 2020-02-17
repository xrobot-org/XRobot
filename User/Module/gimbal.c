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
/* Exported functions --------------------------------------------------------*/
int Gimbal_Init(Gimbal_t *gimb) {
	if (gimb == NULL)
		return -1;
	
	gimb->mode = GIMBAL_MODE_INIT;
	gimb->last_mode = GIMBAL_MODE_INIT;

	PID_Init(&(gimb->yaw_inner_pid), PID_MODE_DERIVATIV_SET, gimb->control_time);
	PID_SetParameters(&(gimb->yaw_inner_pid), 5.f, 1.f, 0.f, 1.f, 1.f);
	
	PID_Init(&(gimb->yaw_outer_pid), PID_MODE_DERIVATIV_NONE, gimb->control_time);
	PID_SetParameters(&(gimb->yaw_outer_pid), 5.f, 1.f, 0.f, 1.f, 1.f);
	
	PID_Init(&(gimb->pit_inner_pid), PID_MODE_DERIVATIV_SET, gimb->control_time);
	PID_SetParameters(&(gimb->pit_inner_pid), 5.f, 1.f, 0.f, 1.f, 1.f);
	
	PID_Init(&(gimb->pit_outer_pid), PID_MODE_DERIVATIV_NONE, gimb->control_time);
	PID_SetParameters(&(gimb->pit_outer_pid), 5.f, 1.f, 0.f, 1.f, 1.f);
	
	return 0;
}

int Gimbal_SetMode(Gimbal_t *gimb, Gimbal_Mode_t mode) {
	if (gimb == NULL)
		return -1;
	
	
	// check mode switchable.
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

int Gimbal_UpdateFeedback(Gimbal_t *gimb, CAN_Device_t *can_device) {
	if (gimb == NULL)
		return -1;
	
	if (can_device == NULL)
		return -1;
	
	const float yaw_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	gimb->yaw_encoder_angle = yaw_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	const float pit_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	gimb->pit_encoder_angle = pit_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	return 0;
}


int Gimbal_Control(Gimbal_t *gimb) {
	if (gimb == NULL)
		return -1;
	
	if (gimb->ctrl_eulr == NULL)
		return -1;
	
	if (gimb->imu == NULL)
		return -1;
	
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
			motor_gyro_set = PID_Calculate(&gimb->yaw_inner_pid, gimb->ctrl_eulr->yaw, gimb->gimb_eulr->yaw, gimb->imu->data.gyro.z, 0.f);
			gimb->yaw_cur_out  = PID_Calculate(&gimb->yaw_outer_pid, motor_gyro_set, gimb->imu->data.gyro.z, 0.f, 0.f);
			
			motor_gyro_set = PID_Calculate(&gimb->pit_inner_pid, gimb->ctrl_eulr->pit, gimb->gimb_eulr->pit, gimb->imu->data.gyro.x, 0.f);
			gimb->pit_cur_out  = PID_Calculate(&gimb->pit_outer_pid, motor_gyro_set, gimb->imu->data.gyro.x, 0.f, 0.f);
			break;
			
		case GIMBAL_MODE_FIX:
			gimb->ctrl_eulr->yaw = 0.f;
			gimb->ctrl_eulr->pit = 0.f;
			/* NO break. */
		case GIMBAL_MODE_RELATIVE:
			motor_gyro_set = PID_Calculate(&gimb->yaw_inner_pid, gimb->ctrl_eulr->yaw, gimb->yaw_encoder_angle, gimb->imu->data.gyro.z, 0.f);
			gimb->yaw_cur_out  = PID_Calculate(&gimb->yaw_outer_pid, motor_gyro_set, gimb->imu->data.gyro.z, 0.f, 0.f);
			
			motor_gyro_set = PID_Calculate(&gimb->pit_inner_pid, gimb->ctrl_eulr->pit, gimb->pit_encoder_angle, gimb->imu->data.gyro.x, 0.f);
			gimb->pit_cur_out  = PID_Calculate(&gimb->pit_outer_pid, motor_gyro_set, gimb->imu->data.gyro.x, 0.f, 0.f);
			break;
			
		default:
			return -1;
	}
	
	// TODO: Lilter output.
	
	return 0;
}
