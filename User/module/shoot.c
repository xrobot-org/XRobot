/* 
	底盘模组

*/


/* Includes ------------------------------------------------------------------*/
#include "shoot.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SHOOT_BULLET_SPEED_SCALER (2.f)
#define SHOOT_BULLET_SPEED_BIAS  (1.f)

#define SHOOT_NUM_TRIG_TOOTH (8u)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function  ---------------------------------------------------------*/ 
static void TrigTimerCallback(void *arg) {
	Shoot_t *s = (Shoot_t*)arg;
	
	s->set.trig_angle += 2.f * M_PI / SHOOT_NUM_TRIG_TOOTH;
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
	}
	return 0;
}

/* Exported functions --------------------------------------------------------*/
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *param, float dt_sec) {
	if (s == NULL)
		return -1;
	
	s->mode = SHOOT_MODE_RELAX;
	s->dt_sec = dt_sec;
	
	s->trig_timer_id = osTimerNew(TrigTimerCallback, osTimerPeriodic, s, NULL);

	for (uint8_t i = 0; i < 2; i++) {
		PID_Init(&(s->pid.fric[i]), PID_MODE_NO_D, s->dt_sec, &(param->fric_pid_param[i]));
		
		LowPassFilter2p_Init(&(s->filter.fric[i]), 1000.f / s->dt_sec, param->low_pass_cutoff.fric);
	}
	
	PID_Init(&(s->pid.trig), PID_MODE_NO_D, s->dt_sec, &(param->trig_pid_param));
	
	LowPassFilter2p_Init(&(s->filter.trig), 1.f / s->dt_sec, param->low_pass_cutoff.trig);
	return 0;
}


int8_t Shoot_UpdateFeedback(Shoot_t *s, CAN_t *can) {
	if (s == NULL)
		return -1;
	
	if (can == NULL)
		return -1;
	
	for (uint8_t i = 0; i < 2; i++) {
		s->fb.fric_rpm[i] = can->shoot_motor_fb[CAN_MOTOR_SHOOT_FRIC1 + i].rotor_speed;
	}
	
	s->fb.trig_angle = can->shoot_motor_fb[CAN_MOTOR_SHOOT_TRIG].rotor_angle;
	
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
	
	s->set.fric_rpm[0] = SHOOT_BULLET_SPEED_SCALER * s_ctrl->bullet_speed + SHOOT_BULLET_SPEED_BIAS;
	s->set.fric_rpm[1] = -s->set.fric_rpm[0];

	
	uint32_t period_ms = 1000u / (uint32_t)s_ctrl->shoot_freq_hz;
	if (!osTimerIsRunning(s->trig_timer_id))
		osTimerStart(s->trig_timer_id, period_ms);  
	
	switch(s->mode) {
		case SHOOT_MODE_RELAX:
			s->out[0] = 0.f;
		
			for (uint8_t i = 0; i < 2; i++) {
				s->out[i + 1] = 0.f;
			}
			break;
		
		case SHOOT_MODE_SAFE:
		case SHOOT_MODE_STDBY:
		case SHOOT_MODE_FIRE:
			s->out[0] = PID_Calc(&(s->pid.trig), s->set.trig_angle, s->fb.trig_angle, 0.f, s->dt_sec);
			s->out[0] = LowPassFilter2p_Apply(&(s->filter.trig), s->out[0]);
		
			for (uint8_t i = 0; i < 2; i++) {
				s->out[i + 1] = PID_Calc(&(s->pid.fric[i + 1]), s->set.fric_rpm[i + 1], s->fb.fric_rpm[i + 1], 0.f, s->dt_sec);
				s->out[i + 1] = LowPassFilter2p_Apply(&(s->filter.fric[i + 1]), s->out[i + 1]);
			}
			break;
	}
	return 0;
}
