/* 
	底盘模组

*/


/* Includes ------------------------------------------------------------------*/
#include "chassis.h"

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
int Chassis_Init(Chassis_t *chas, Chassis_Type_t type) {
	if (chas == NULL)
		return CHASSIS_ERR_NULL;
	
	chas->mode = CHASSIS_MODE_RELAX;
	chas->type = type;
	
	for(uint8_t i = 0; i < 4; i++) {
		PID_Init(&(chas->wheel_pid[i]), PID_MODE_DERIVATIV_NONE, chas->dt_sec);
		PID_SetParameters(&(chas->wheel_pid[i]), 5.f, 1.f, 0.f, 1.f, 1.f);
	}
	
	switch (type) {
		case CHASSIS_TYPE_MECANUM:
			chas->wheel_num = 4;
			chas->Mix = Mixer_Mecanum;
			break;
		
		case CHASSIS_MODE_PARLFIX4:
			chas->wheel_num = 4;
			chas->Mix = Mixer_ParlFix4;
			break;
		
		case CHASSIS_MODE_PARLFIX2:
			chas->wheel_num = 2;
			chas->Mix = Mixer_ParlFix2;
			break;
		
		case CHASSIS_MODE_OMNI_CROSS:
			chas->wheel_num = 4;
			chas->Mix = Mixer_OmniCross;
			break;
		
		case CHASSIS_MODE_OMNI_PLUS:
			chas->wheel_num = 4;
			chas->Mix = Mixer_OmniPlus;
			break;
		
		case CHASSIS_MODE_DRONE:
			// onboard sdk.
			return CHASSIS_ERR_TYPE;
	}
	return CHASSIS_OK;
}

int Chassis_SetMode(Chassis_t *chas, Chassis_Mode_t mode) {
	if (chas == NULL)
		return CHASSIS_ERR_NULL;
	
	if (mode == chas->mode)
		return CHASSIS_OK;
	
	// check mode switchable.
	switch (mode) {
		case CHASSIS_MODE_RELAX:
			break;
		
		case CHASSIS_MODE_BREAK:
			break;
		
		case CHASSIS_MODE_FOLLOW_GIMBAL:
			PID_Init(&(chas->follow_pid), PID_MODE_DERIVATIV_NONE, chas->dt_sec);
			PID_SetParameters(&(chas->follow_pid), 5.f, 1.f, 0.f, 1.f, 1.f);

			// TODO
		
			break;
		case CHASSIS_MODE_ROTOR:
			break;
		
		case CHASSIS_MODE_INDENPENDENT:
			// TODO
			break;
		
		case CHASSIS_MODE_OPEN:
			break;
		
		default:
			return CHASSIS_ERR_MODE;
	}
	return CHASSIS_OK;
}

int Chassis_UpdateFeedback(Chassis_t *chas, CAN_Device_t *can_device) {
	if (chas == NULL)
		return CHASSIS_ERR_NULL;
	
	if (can_device == NULL)
		return CHASSIS_ERR_NULL;
	
	const float raw_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	chas->gimbal_yaw_angle = raw_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	for(uint8_t i = 0; i < 4; i++) {
		const float raw_peed = can_device->chassis_motor_fb[i].rotor_speed;
		// TODO.
		chas->motor_speed[i] = raw_peed;
	}
	
	return CHASSIS_OK;
}

int Chassis_ParseCommand(Chassis_Ctrl_t *chas_ctrl, const DR16_t *dr16) {
	if (chas_ctrl == NULL)
		return CHASSIS_ERR_NULL;
	
	if (dr16 == NULL)
		return CHASSIS_ERR_NULL;
	
	//TODO
	
	return CHASSIS_OK;
}

int Chassis_Control(Chassis_t *chas, const Chassis_MoveVector_t *ctrl_v) {
	if (chas == NULL)
		return CHASSIS_ERR_NULL;
	
	if (ctrl_v == NULL)
		return CHASSIS_ERR_NULL;
	
	if (chas->Mix == NULL)
		return CHASSIS_ERR_NULL;
	
	/* robot_ctrl_v -> chas_v. */
	/* Compute vx and vy. */
	if (chas->mode == CHASSIS_MODE_BREAK) {
		chas->chas_v.vx = 0.f;
		chas->chas_v.vy = 0.f;
		
	} else {
		const float cos_beta = cosf(chas->gimbal_yaw_angle);
		const float sin_beta = sinf(chas->gimbal_yaw_angle);
		
		chas->chas_v.vx = cos_beta * ctrl_v->vx - sin_beta * ctrl_v->vy;
		chas->chas_v.vy = sin_beta * ctrl_v->vx - cos_beta * ctrl_v->vy;
	}
	
	/* Compute wz. */
	if (chas->mode == CHASSIS_MODE_BREAK) {
		chas->chas_v.wz = 0.f;
		
	} else if (chas->mode == CHASSIS_MODE_FOLLOW_GIMBAL) {
		chas->chas_v.wz = PID_Calculate(&(chas->follow_pid), 0, chas->gimbal_yaw_angle, 0.f, chas->dt_sec);
		
	} else if (chas->mode == CHASSIS_MODE_ROTOR) {
		chas->chas_v.wz = 0.8;
	}
	
	/* chas_v -> motor_rpm_set. */
	chas->Mix(
		chas->chas_v.vx, 
		chas->chas_v.vy,
		chas->chas_v.wz,
		chas->motor_rpm_set,
		chas->wheel_num);
	
	/* motor_rpm_set -> motor_cur_out. */
	for(uint8_t i = 0; i < 4; i++) {
		switch(chas->mode) {
			case CHASSIS_MODE_BREAK:
			case CHASSIS_MODE_FOLLOW_GIMBAL:
			case CHASSIS_MODE_ROTOR:
			case CHASSIS_MODE_INDENPENDENT:
				chas->motor_cur_out[i] = PID_Calculate(&(chas->wheel_pid[i]), chas->motor_rpm_set[i], chas->motor_speed[i], 0.f, chas->dt_sec);
				break;
				
			case CHASSIS_MODE_OPEN:
				chas->motor_cur_out[i] = chas->motor_rpm_set[i];
				break;
				
			case CHASSIS_MODE_RELAX:
				chas->motor_cur_out[i] = 0;
				break;
			
			default:
				return -1;
		}
	}
	
	// TODO: Filter output.
	
	
	return CHASSIS_OK;
}
