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
static void TrigTimerCallback  (void const *arg) {
	Shoot_t *shoot = (Shoot_t*)arg;
	
	shoot->trig_pos_set += 2.f * PI / SHOOT_FEEDING_TOOTH_NUM;
	
}

/* Exported functions --------------------------------------------------------*/
osTimerDef(trig_timer, TrigTimerCallback);

int Shoot_Init(Shoot_t *shoot) {
	if (shoot == NULL)
		return -1;
	
	shoot->mode = SHOOT_MODE_RELAX;
	
	shoot->trig_timer_id = osTimerCreate(osTimer(trig_timer), osTimerPeriodic, shoot);

	for(uint8_t i = 0; i < 2; i++) {
		PID_Init(&(shoot->fric_pid[i]), PID_MODE_DERIVATIV_NONE, shoot->dt_sec);
		PID_SetParameters(&(shoot->fric_pid[i]), 5.f, 1.f, 0.f, 1.f, 1.f);
	}
	
	PID_Init(&(shoot->trig_pid), PID_MODE_DERIVATIV_NONE, shoot->dt_sec);
	PID_SetParameters(&(shoot->trig_pid), 5.f, 1.f, 0.f, 1.f, 1.f);
	
	return 0;
}

int Shoot_SetMode(Shoot_t *shoot, Shoot_Mode_t mode) {
	if (shoot == NULL)
		return -1;
	
	
	// check mode switchable.
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

int Shoot_UpdateFeedback(Shoot_t *shoot, CAN_Device_t *can_device) {
	if (shoot == NULL)
		return -1;
	
	if (can_device == NULL)
		return -1;
	
	for(uint8_t i = 0; i < 2; i++) {
		const float fric_speed = can_device->gimbal_motor_fb.fric_fb[i].rotor_speed;
		shoot->fric_rpm[i] = fric_speed;
	}
	
	const float trig_angle = can_device->gimbal_motor_fb.yaw_fb.rotor_angle;
	shoot->trig_angle = trig_angle / (float)CAN_MOTOR_MAX_ENCODER * 2.f * PI;
	
	return 0;
}
int Shoot_ParseCommand(Shoot_Ctrl_t *shoot_ctrl, const DR16_t *dr16) {
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
		if (dr16->data.mouse.left_click) {
			if (dr16->data.mouse.right_click) {
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

int Shoot_Control(Shoot_t *shoot, float bullet_speed, uint32_t shoot_freq_hz) {
	if (shoot == NULL)
		return -1;
	
	if (shoot->mode == SHOOT_MODE_SAFE) {
		bullet_speed = 0.f;
		shoot_freq_hz = 0.f;
	}
	
	shoot->fric_rpm_set[0] = -SHOOT_BULLET_SPEED_SCALER * bullet_speed - SHOOT_BULLET_SPEED_BIAS;
	shoot->fric_rpm_set[1] = SHOOT_BULLET_SPEED_SCALER * bullet_speed + SHOOT_BULLET_SPEED_BIAS;
	
	uint32_t period_ms = 1000u / shoot_freq_hz;
	osTimerStart(shoot->trig_timer_id, period_ms);  
	
	
	switch(shoot->mode) {
		case SHOOT_MODE_RELAX:
			for(uint8_t i = 0; i < 2; i++) {
				shoot->fric_cur_out[i] = 0.f;
			}
			shoot->trig_cur_out = 0.f;
			break;
		
		case SHOOT_MODE_SAFE:
		case SHOOT_MODE_STDBY:
		case SHOOT_MODE_FIRE:
			for(uint8_t i = 0; i < 2; i++) {
				shoot->fric_cur_out[i] = PID_Calculate(&(shoot->fric_pid[i]), shoot->fric_rpm_set[i], shoot->fric_rpm[i], 0.f, shoot->dt_sec);
			}
			shoot->trig_cur_out = PID_Calculate(&(shoot->trig_pid), shoot->trig_pos_set, shoot->trig_angle, 0.f, shoot->dt_sec);
			break;
		
		default:
			return -1;
	}
	
	// TODO: Filter output.
	return 0;
}
