/* 
	底盘模组

*/


/* Includes ------------------------------------------------------------------*/
#include "shoot.h"

#include "component\user_math.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/ 
static void TrigTimerCallback  (void *arg) {
	Shoot_t *s = (Shoot_t*)arg;
	
	s->trig_angle_set += 2.f * PI / SHOOT_NUM_FEEDING_TOOTH;
}

static int8_t Shoot_SetMode(Shoot_t *s, CMD_Shoot_Mode_t mode) {
	if (s == NULL)
		return -1;
	
	if (mode == s->mode)
		return SHOOT_OK;
	
	s->mode = mode;
	
	// TODO: Check mode switchable.
	switch (mode) {
		case SHOOT_MODE_RELAX:
			break;
		
		case SHOOT_MODE_SAFE:
			break;
		
		case SHOOT_MODE_STDBY:
			break;
		
		case SHOOT_MODE_FIRE:
			break;
		
		default:
			return -1;
	}
	return 0;
}

/* Exported functions --------------------------------------------------------*/
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *param, float32_t dt_sec) {
	if (s == NULL)
		return -1;
	
	s->mode = SHOOT_MODE_RELAX;
	s->dt_sec = dt_sec;
	
	s->trig_timer_id = osTimerNew(TrigTimerCallback, osTimerPeriodic, s, NULL);

	for(uint8_t i = 0; i < 2; i++) {
		PID_Init(&(s->fric_pid[i]), PID_MODE_NO_D, s->dt_sec, &(param->fric_pid_param[i]));
		
		LowPassFilter2p_Init(&(s->fric_output_filter[i]), 1000.f / s->dt_sec, 100.f);
	}
	
	PID_Init(&(s->trig_pid), PID_MODE_NO_D, s->dt_sec, &(param->trig_pid_param));
	
	LowPassFilter2p_Init(&(s->trig_output_filter), 1.f / s->dt_sec, 100.f);
	return 0;
}


int8_t Shoot_UpdateFeedback(Shoot_t *s, CAN_Device_t *can_device) {
	if (s == NULL)
		return -1;
	
	if (can_device == NULL)
		return -1;
	
	for(uint8_t i = 0; i < 2; i++) {
		const float32_t fric_speed = can_device->gimbal_motor_fb.fric_fb[i].rotor_speed;
		s->fric_rpm[i] = fric_speed;
	}
	
	const float32_t trig_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	s->trig_angle = trig_angle / (float32_t)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	return 0;
}

int8_t Shoot_Control(Shoot_t *s, CMD_Shoot_Ctrl_t *s_ctrl) {
	if (s == NULL)
		return -1;
	
	Shoot_SetMode(s, s_ctrl->mode);
	
	if (s->mode == SHOOT_MODE_SAFE) {
		s_ctrl->bullet_speed = 0.f;
		s_ctrl->shoot_freq_hz = 0.f;
	}
	
	s->fric_rpm_set[0] = SHOOT_BULLET_SPEED_SCALER * s_ctrl->bullet_speed + SHOOT_BULLET_SPEED_BIAS;
	s->fric_rpm_set[1] = -s->fric_rpm_set[0];

	
	uint32_t period_ms = 1000u / s_ctrl->shoot_freq_hz;
	if (!osTimerIsRunning(s->trig_timer_id))
		osTimerStart(s->trig_timer_id, period_ms);  
	
	switch(s->mode) {
		case SHOOT_MODE_RELAX:
			s->trig_cur_out = 0.f;
		
			for(uint8_t i = 0; i < 2; i++) {
				s->fric_cur_out[i] = 0.f;
			}
			break;
		
		case SHOOT_MODE_SAFE:
		case SHOOT_MODE_STDBY:
		case SHOOT_MODE_FIRE:
			s->trig_cur_out = PID_Calc(&(s->trig_pid), s->trig_angle_set, s->trig_angle, 0.f, s->dt_sec);
			s->trig_cur_out = LowPassFilter2p_Apply(&(s->trig_output_filter), s->trig_cur_out);
		
			for(uint8_t i = 0; i < 2; i++) {
				s->fric_cur_out[i] = PID_Calc(&(s->fric_pid[i]), s->fric_rpm_set[i], s->fric_rpm[i], 0.f, s->dt_sec);
				s->fric_cur_out[i] = LowPassFilter2p_Apply(&(s->fric_output_filter[i]), s->fric_cur_out[i]);
			}
			break;
		
		default:
			return -1;
	}
	return 0;
}
