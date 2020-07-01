/* 
	控制命令
*/

#include "cmd.h"

bool CMD_KeyPressed(const CMD_RC_t *rc, CMD_KeyValue_t key) {
	return rc->key & (1u << key);
}

int8_t CMD_Parse(const CMD_RC_t *rc, CMD_t *cmd) {
	/* RC Control. */
	switch (rc->sw_l) {
		case CMD_SW_UP:
		case CMD_SW_MID:
		case CMD_SW_DOWN:
			cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;
			break;
		
		case CMD_SW_ERR:
			cmd->gimbal.mode = GIMBAL_MODE_RELAX;
			break;
	}
	
	cmd->gimbal.eulr.yaw += rc->ch_r_x;
	cmd->gimbal.eulr.pit += rc->ch_r_y;
	
	if ((rc->sw_l == CMD_SW_UP) && (rc->sw_r == CMD_SW_UP)) {
		/* PC Control. */
		cmd->gimbal.eulr.yaw += (float32_t)rc->mouse.x / 100.f;	
		cmd->gimbal.eulr.pit += (float32_t)rc->mouse.y / 100.f;
		
	}
	
		
	/* RC Control. */
	switch (rc->sw_l) {
		case CMD_SW_UP:
			cmd->chassis.mode = CHASSIS_MODE_BREAK;
			break;
		
		case CMD_SW_MID:
			cmd->chassis.mode = CHASSIS_MODE_FOLLOW_GIMBAL;
			break;
		
		case CMD_SW_DOWN:
			cmd->chassis.mode = CHASSIS_MODE_ROTOR;
			break;
		
		case CMD_SW_ERR:
			cmd->chassis.mode = CHASSIS_MODE_RELAX;
			break;
	}
	
	cmd->chassis.ctrl_v.vx = rc->ch_l_x;
	cmd->chassis.ctrl_v.vy = rc->ch_l_y;
	
	if ((rc->sw_l == CMD_SW_UP) && (rc->sw_r == CMD_SW_UP)) {
		/* PC Control. */
		
		cmd->chassis.ctrl_v.vx = 0.f;
		cmd->chassis.ctrl_v.vy = 0.f;
		if (!CMD_KeyPressed(rc, DR16_KEY_SHIFT) && !CMD_KeyPressed(rc, DR16_KEY_CTRL)) {
			if (CMD_KeyPressed(rc, DR16_KEY_A))
				cmd->chassis.ctrl_v.vx -= 1.f;
			
			if (CMD_KeyPressed(rc, DR16_KEY_D))
				cmd->chassis.ctrl_v.vx += 1.f;
			
			if (CMD_KeyPressed(rc, DR16_KEY_W))
				cmd->chassis.ctrl_v.vy += 1.f;
			
			if (CMD_KeyPressed(rc, DR16_KEY_S))
				cmd->chassis.ctrl_v.vy -= 1.f;
		}
	}
	
		/* RC Control. */
	switch (rc->sw_r) {
		case CMD_SW_UP:
			cmd->shoot.mode = SHOOT_MODE_SAFE;
			break;
		
		case CMD_SW_MID:
			cmd->shoot.mode = SHOOT_MODE_STDBY;
			break;
		
		case CMD_SW_DOWN:
			cmd->shoot.mode = SHOOT_MODE_FIRE;
			cmd->shoot.shoot_freq_hz = 10u;
			cmd->shoot.bullet_speed = 10.f;
			break;
		
		case CMD_SW_ERR:
			cmd->shoot.mode = SHOOT_MODE_RELAX;
			break;
	}
	
	if ((rc->sw_l == CMD_SW_UP) && (rc->sw_r == CMD_SW_UP)) {
		/* PC Control. */
		if (rc->mouse.l_click) {
			if (rc->mouse.r_click) {
				cmd->shoot.shoot_freq_hz = 5u;
				cmd->shoot.bullet_speed = 20.f;
			} else {
				cmd->shoot.shoot_freq_hz = 10u;
				cmd->shoot.bullet_speed = 10.f;
			}
		} else {
			cmd->shoot.shoot_freq_hz = 0u;
			cmd->shoot.bullet_speed = 0.f;
		}
		
		if (CMD_KeyPressed(rc, DR16_KEY_SHIFT) && CMD_KeyPressed(rc, DR16_KEY_CTRL)) {
			if (CMD_KeyPressed(rc, DR16_KEY_A))
				cmd->shoot.mode = SHOOT_MODE_SAFE;
			
			else if (CMD_KeyPressed(rc, DR16_KEY_S))
				cmd->shoot.mode = SHOOT_MODE_STDBY;
			
			else if (CMD_KeyPressed(rc, DR16_KEY_D))
				cmd->shoot.mode = SHOOT_MODE_FIRE;
			
			else
				cmd->shoot.mode = SHOOT_MODE_RELAX;
		}
	}
	return 0;
}
