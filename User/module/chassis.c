/* 
	底盘模组

*/


/* Includes ------------------------------------------------------------------*/
#include "chassis.h"

#include "bsp\mm.h"

#include "component\user_math.h"
#include "component\limiter.h"

#include "device\can.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/
static int8_t Chassis_SetMode(Chassis_t *c, CMD_Chassis_Mode_t mode) {
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
	}
	return CHASSIS_OK;
}

/* Exported functions --------------------------------------------------------*/
int8_t Chassis_Init(Chassis_t *c, const Chassis_Params_t *param, float dt_sec) {
	if (c == NULL)
		return CHASSIS_ERR_NULL;
	
	c->dt_sec = dt_sec;
	
	c->mode = CHASSIS_MODE_RELAX;
	Mixer_Mode_t mixer_mode;
	switch (param->type) {
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
	
	c->motor_rpm = BSP_Malloc((size_t)c->num_wheel * sizeof(*c->motor_rpm));
	if (c->motor_rpm == NULL)
		goto error1;
	
	c->motor_rpm_set = BSP_Malloc((size_t)c->num_wheel * sizeof(*c->motor_rpm_set));
	if (c->motor_rpm_set == NULL)
		goto error2;
	
	c->motor_pid = BSP_Malloc((size_t)c->num_wheel * sizeof(*c->motor_pid));
	if (c->motor_pid == NULL)
		goto error3;
	
	c->motor_cur_out = BSP_Malloc((size_t)c->num_wheel * sizeof(*c->motor_cur_out));
	if (c->motor_cur_out == NULL)
		goto error4;
	
	c->output_filter = BSP_Malloc((size_t)c->num_wheel * sizeof(*c->output_filter));
	if (c->output_filter == NULL)
		goto error5;
		
	for (uint8_t i = 0; i < c->num_wheel; i++) {
		PID_Init(&(c->motor_pid[i]), PID_MODE_NO_D, c->dt_sec, &(param->motor_pid_param[i]));
		
		LowPassFilter2p_Init(&(c->output_filter[i]), 1.f / c->dt_sec, 100.f);
	}
	
	PID_Init(&(c->follow_pid), PID_MODE_NO_D, c->dt_sec, &(param->follow_pid_param));
	
	Mixer_Init(&(c->mixer), mixer_mode);
	c->motor_scaler = CAN_M3508_MAX_ABS_VOLT;
	
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


int8_t Chassis_UpdateFeedback(Chassis_t *c, CAN_t *can) {
	if (c == NULL)
		return CHASSIS_ERR_NULL;
	
	if (can == NULL)
		return CHASSIS_ERR_NULL;
	
	const float raw_angle = can->gimbal_motor_fb.yaw_fb.rotor_angle;
	c->gimbal_yaw_angle = raw_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * M_PI;
	
	for (uint8_t i = 0; i < 4; i++) {
		const float raw_speed = can->chassis_motor_fb[i].rotor_speed;
		c->motor_rpm[i] = raw_speed; // TODO
	}
	
	return CHASSIS_OK;
}

int8_t Chassis_Control(Chassis_t *c, CMD_Chassis_Ctrl_t *c_ctrl) {
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
		const float cos_beta = cosf(c->gimbal_yaw_angle);
		const float sin_beta = sinf(c->gimbal_yaw_angle);
		
		c->move_vec.vx = cos_beta * c_ctrl->ctrl_v.vx - sin_beta * c_ctrl->ctrl_v.vy;
		c->move_vec.vy = sin_beta * c_ctrl->ctrl_v.vx - cos_beta * c_ctrl->ctrl_v.vy;
	}
	
	/* Compute wz. */
	if (c->mode == CHASSIS_MODE_BREAK) {
		c->move_vec.wz = 0.f;
		
	} else if (c->mode == CHASSIS_MODE_FOLLOW_GIMBAL) {
		c->move_vec.wz = PID_Calc(&(c->follow_pid), 0, c->gimbal_yaw_angle, 0.f, c->dt_sec);
		
	} else if (c->mode == CHASSIS_MODE_ROTOR) {
		c->move_vec.wz = 0.8f;
	}
	
	/* move_vec -> motor_rpm_set. */
	Mixer_Apply(&(c->mixer), c->move_vec.vx, c->move_vec.vy, c->move_vec.wz, c->motor_rpm_set, c->num_wheel);
	
	/* Compute output from setpiont. */
	for (uint8_t i = 0; i < 4; i++) {
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
		}
		
		/* Filter output. */
		c->motor_cur_out[i] = LowPassFilter2p_Apply(&(c->output_filter[i]), c->motor_cur_out[i]);
	}
	return CHASSIS_OK;
}
