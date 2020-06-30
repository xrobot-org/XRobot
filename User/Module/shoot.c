/* 
	底盘模组

*/


/* Includes ------------------------------------------------------------------*/
#include "shoot.h"

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
static void TrigTimerCallback  (void *arg) {
	Shoot_t *s = (Shoot_t*)arg;
	
	s->trig_angle_set += 2.f * PI / SHOOT_NUM_FEEDING_TOOTH;
}

static int8_t Shoot_SetMode(Shoot_t *s, Shoot_Mode_t mode) {
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
int8_t Shoot_Init(Shoot_t *s, const Shoot_Params_t *shoot_param) {
	if (s == NULL)
		return -1;
	
	s->mode = SHOOT_MODE_RELAX;
	
	s->trig_timer_id = osTimerNew(TrigTimerCallback, osTimerPeriodic, s, NULL);

	for(uint8_t i = 0; i < 2; i++) {
		PID_Init(&(s->fric_pid[i]), PID_MODE_NO_D, s->dt_sec, &(shoot_param->fric_pid_param[i]));
		
		LowPassFilter2p_Init(&(s->fric_output_filter[i]), 1000.f / s->dt_sec, 100.f);
	}
	
	PID_Init(&(s->trig_pid), PID_MODE_NO_D, s->dt_sec, &(shoot_param->trig_pid_param));
	
	LowPassFilter2p_Init(&(s->trig_output_filter), 1000.f / s->dt_sec, 100.f);
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
int8_t Shoot_ParseCommand(Shoot_Ctrl_t *shoot_ctrl, const DR16_t *dr16) {
	if (shoot_ctrl == NULL)
		return -1;
	
	if (dr16 == NULL)
		return -1;

	
	/* RC Control. */
	switch (dr16->data.rc.sw_r) {
		case DR16_SW_UP:
			shoot_ctrl->mode = SHOOT_MODE_SAFE;
			break;
		
		case DR16_SW_MID:
			shoot_ctrl->mode = SHOOT_MODE_STDBY;
			break;
		
		case DR16_SW_DOWN:
			shoot_ctrl->mode = SHOOT_MODE_FIRE;
			shoot_ctrl->shoot_freq_hz = 10u;
			shoot_ctrl->bullet_speed = 10.f;
			break;
		
		case DR16_SW_ERR:
			shoot_ctrl->mode = SHOOT_MODE_RELAX;
			break;
	}
	
	if ((dr16->data.rc.sw_l == DR16_SW_UP) && (dr16->data.rc.sw_r == DR16_SW_UP)) {
		/* PC Control. */
		if (dr16->data.mouse.l_click) {
			if (dr16->data.mouse.r_click) {
				shoot_ctrl->shoot_freq_hz = 5u;
				shoot_ctrl->bullet_speed = 20.f;
			} else {
				shoot_ctrl->shoot_freq_hz = 10u;
				shoot_ctrl->bullet_speed = 10.f;
			}
		} else {
			shoot_ctrl->shoot_freq_hz = 0u;
			shoot_ctrl->bullet_speed = 0.f;
		}
		
		if (DR16_KeyPressed(dr16, DR16_KEY_SHIFT) && DR16_KeyPressed(dr16, DR16_KEY_CTRL)) {
			if (DR16_KeyPressed(dr16, DR16_KEY_A))
				shoot_ctrl->mode = SHOOT_MODE_SAFE;
			
			else if (DR16_KeyPressed(dr16, DR16_KEY_S))
				shoot_ctrl->mode = SHOOT_MODE_STDBY;
			
			else if (DR16_KeyPressed(dr16, DR16_KEY_D))
				shoot_ctrl->mode = SHOOT_MODE_FIRE;
			
			else
				shoot_ctrl->mode = SHOOT_MODE_RELAX;
		}
	}
	return -1;
}

int8_t Shoot_Control(Shoot_t *s, Shoot_Ctrl_t *shoot_ctrl) {
	if (s == NULL)
		return -1;
	
	Shoot_SetMode(s, shoot_ctrl->mode);
	
	if (s->mode == SHOOT_MODE_SAFE) {
		shoot_ctrl->bullet_speed = 0.f;
		shoot_ctrl->shoot_freq_hz = 0.f;
	}
	
	s->fric_rpm_set[0] = SHOOT_BULLET_SPEED_SCALER * shoot_ctrl->bullet_speed + SHOOT_BULLET_SPEED_BIAS;
	s->fric_rpm_set[1] = -s->fric_rpm_set[0];

	
	uint32_t period_ms = 1000u / shoot_ctrl->shoot_freq_hz;
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
