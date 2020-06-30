/* 
	底盘模组

*/


/* Includes ------------------------------------------------------------------*/
#include "chassis.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
#include "bsp_mm.h"

/* Include Device相关的头文件 */
#include "can_device.h"

/* Include Component相关的头文件 */
#include "user_math.h"
#include "limiter.h"

/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
static int8_t Chassis_SetMode(Chassis_t *c, Chassis_Mode_t mode) {
	if (c == NULL)
		return CHASSIS_ERR_NULL;
	
	if (mode == c->mode)
		return CHASSIS_OK;
	
	c->mode = mode;
	
	// TODO: Check mode switchable.
	switch (mode) {
		case CHASSIS_MODE_RELAX:
			break;
		
		case CHASSIS_MODE_BREAK:
			break;
		
		case CHASSIS_MODE_FOLLOW_GIMBAL:
			break;
		
		case CHASSIS_MODE_ROTOR:
			break;
		
		case CHASSIS_MODE_INDENPENDENT:
			break;
		
		case CHASSIS_MODE_OPEN:
			break;
		
		default:
			return CHASSIS_ERR_MODE;
	}
	return CHASSIS_OK;
}

/* Exported functions --------------------------------------------------------*/
int8_t Chassis_Init(Chassis_t *c, const Chassis_Params_t *chas_param) {
	if (c == NULL)
		return CHASSIS_ERR_NULL;
	
	c->mode = CHASSIS_MODE_RELAX;
	Mixer_Mode_t mixer_mode;
	switch (chas_param->type) {
		case CHASSIS_TYPE_MECANUM:
			c->num_wheel = 4;
			mixer_mode = MIXER_MECANUM;
			break;
		
		case CHASSIS_MODE_PARLFIX4:
			c->num_wheel = 4;
			mixer_mode = MIXER_PARLFIX4;
			break;
		
		case CHASSIS_MODE_PARLFIX2:
			c->num_wheel = 2;
			mixer_mode = MIXER_PARLFIX2;
			break;
		
		case CHASSIS_MODE_OMNI_CROSS:
			c->num_wheel = 4;
			mixer_mode = MIXER_OMNICROSS;
			break;
		
		case CHASSIS_MODE_OMNI_PLUS:
			c->num_wheel = 4;
			mixer_mode = MIXER_OMNIPLUS;
			break;
		
		case CHASSIS_MODE_DRONE:
			// onboard sdk.
			return CHASSIS_ERR_TYPE;
	}
	
	c->motor_rpm = BSP_Malloc(c->num_wheel * sizeof(*c->motor_rpm));
	if(c->motor_rpm == NULL)
		goto error1;
	
	c->motor_rpm_set = BSP_Malloc(c->num_wheel * sizeof(*c->motor_rpm_set));
	if(c->motor_rpm_set == NULL)
		goto error2;
	
	c->motor_pid = BSP_Malloc(c->num_wheel * sizeof(*c->motor_pid));
	if(c->motor_pid == NULL)
		goto error3;
	
	c->motor_cur_out = BSP_Malloc(c->num_wheel * sizeof(*c->motor_cur_out));
	if(c->motor_cur_out == NULL)
		goto error4;
	
	c->output_filter = BSP_Malloc(c->num_wheel * sizeof(*c->output_filter));
	if(c->output_filter == NULL)
		goto error5;
		
	for(uint8_t i = 0; i < c->num_wheel; i++) {
		PID_Init(&(c->motor_pid[i]), PID_MODE_NO_D, c->dt_sec, &(chas_param->motor_pid_param[i]));
		
		LowPassFilter2p_Init(&(c->output_filter[i]), 1000.f / c->dt_sec, 100.f);
	}
	
	PID_Init(&(c->follow_pid), PID_MODE_NO_D, c->dt_sec, &(chas_param->follow_pid_param));
	
	Mixer_Init(&(c->mixer), mixer_mode);
	c->motor_scaler = CAN_M3508_MAX_ABS_VOLTAGE;
	
	return CHASSIS_OK;
	
error5:
	BSP_Free(c->motor_cur_out);
error4:
	BSP_Free(c->motor_pid);
error3:
	BSP_Free(c->motor_rpm_set);
error2:
	BSP_Free(c->motor_rpm);
error1:
	return CHASSIS_ERR_NULL;
}


int8_t Chassis_UpdateFeedback(Chassis_t *c, CAN_Device_t *can_device) {
	if (c == NULL)
		return CHASSIS_ERR_NULL;
	
	if (can_device == NULL)
		return CHASSIS_ERR_NULL;
	
	const float32_t raw_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	c->gimbal_yaw_angle = raw_angle / (float32_t)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	for(uint8_t i = 0; i < 4; i++) {
		const float32_t raw_speed = can_device->chassis_motor_fb[i].rotor_speed;
		c->motor_rpm[i] = raw_speed; // TODO
	}
	
	return CHASSIS_OK;
}

int8_t Chassis_ParseCommand(Chassis_Ctrl_t *c_ctrl, const DR16_t *dr16) {
	if (c_ctrl == NULL)
		return CHASSIS_ERR_NULL;
	
	if (dr16 == NULL)
		return CHASSIS_ERR_NULL;
	
	/* RC Control. */
	switch (dr16->data.rc.sw_l) {
		case DR16_SW_UP:
			c_ctrl->mode = CHASSIS_MODE_BREAK;
			break;
		
		case DR16_SW_MID:
			c_ctrl->mode = CHASSIS_MODE_FOLLOW_GIMBAL;
			break;
		
		case DR16_SW_DOWN:
			c_ctrl->mode = CHASSIS_MODE_ROTOR;
			break;
		
		case DR16_SW_ERR:
			c_ctrl->mode = CHASSIS_MODE_RELAX;
			break;
	}
	
	c_ctrl->ctrl_v.vx = dr16->data.rc.ch_l_x;
	c_ctrl->ctrl_v.vy = dr16->data.rc.ch_l_y;
	
	if ((dr16->data.rc.sw_l == DR16_SW_UP) && (dr16->data.rc.sw_r == DR16_SW_UP)) {
		/* PC Control. */
		
		c_ctrl->ctrl_v.vx = 0.f;
		c_ctrl->ctrl_v.vy = 0.f;
		if (!DR16_KeyPressed(dr16, DR16_KEY_SHIFT) && !DR16_KeyPressed(dr16, DR16_KEY_CTRL)) {
			if (DR16_KeyPressed(dr16, DR16_KEY_A))
				c_ctrl->ctrl_v.vx -= 1.f;
			
			if (DR16_KeyPressed(dr16, DR16_KEY_D))
				c_ctrl->ctrl_v.vx += 1.f;
			
			if (DR16_KeyPressed(dr16, DR16_KEY_W))
				c_ctrl->ctrl_v.vy += 1.f;
			
			if (DR16_KeyPressed(dr16, DR16_KEY_S))
				c_ctrl->ctrl_v.vy -= 1.f;
		}
	}
	
	return CHASSIS_OK;
}

int8_t Chassis_Control(Chassis_t *c, Chassis_Ctrl_t *c_ctrl) {
	if (c == NULL)
		return CHASSIS_ERR_NULL;
	
	if (c_ctrl == NULL)
		return CHASSIS_ERR_NULL;
	
	Chassis_SetMode(c, c_ctrl->mode);
	
	/* ctrl_v -> move_vec. */
	/* Compute vx and vy. */
	if (c->mode == CHASSIS_MODE_BREAK) {
		c->move_vec.vx = 0.f;
		c->move_vec.vy = 0.f;
		
	} else {
		const float32_t cos_beta = cosf(c->gimbal_yaw_angle);
		const float32_t sin_beta = sinf(c->gimbal_yaw_angle);
		
		c->move_vec.vx = cos_beta * c_ctrl->ctrl_v.vx - sin_beta * c_ctrl->ctrl_v.vy;
		c->move_vec.vy = sin_beta * c_ctrl->ctrl_v.vx - cos_beta * c_ctrl->ctrl_v.vy;
	}
	
	/* Compute wz. */
	if (c->mode == CHASSIS_MODE_BREAK) {
		c->move_vec.wz = 0.f;
		
	} else if (c->mode == CHASSIS_MODE_FOLLOW_GIMBAL) {
		c->move_vec.wz = PID_Calc(&(c->follow_pid), 0, c->gimbal_yaw_angle, 0.f, c->dt_sec);
		
	} else if (c->mode == CHASSIS_MODE_ROTOR) {
		c->move_vec.wz = 0.8;
	}
	
	/* move_vec -> motor_rpm_set. */
	Mixer_Apply(&(c->mixer), c->move_vec.vx, c->move_vec.vy, c->move_vec.wz, c->motor_rpm_set, c->num_wheel);
	
	/* Compute output from setpiont. */
	for(uint8_t i = 0; i < 4; i++) {
		switch(c->mode) {
			case CHASSIS_MODE_BREAK:
			case CHASSIS_MODE_FOLLOW_GIMBAL:
			case CHASSIS_MODE_ROTOR:
			case CHASSIS_MODE_INDENPENDENT:
				c->motor_cur_out[i] = PID_Calc(&(c->motor_pid[i]), c->motor_rpm_set[i], c->motor_rpm[i], 0.f, c->dt_sec);
				break;
				
			case CHASSIS_MODE_OPEN:
				c->motor_cur_out[i] = c->motor_rpm_set[i];
				break;
				
			case CHASSIS_MODE_RELAX:
				c->motor_cur_out[i] = 0;
				break;
			
			default:
				return -1;
		}
		
		/* Filter output. */
		c->motor_cur_out[i] = LowPassFilter2p_Apply(&(c->output_filter[i]), c->motor_cur_out[i]);
	}
	
	
	return CHASSIS_OK;
}
