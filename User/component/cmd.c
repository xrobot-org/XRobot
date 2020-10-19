/*
  控制命令
*/

#include "cmd.h"

static bool CMD_KeyPressed(const CMD_RC_t *rc, CMD_KeyValue_t key) {
  return rc->key & (1u << key);
}

int8_t CMD_Init(CMD_t *cmd, const CMD_Params_t *param) {
  if (cmd == NULL) return -1;

  cmd->pc_ctrl = false;
  cmd->param = param;

  return 0;
}

int8_t CMD_Parse(const CMD_RC_t *rc, CMD_t *cmd) {
  if (rc == NULL) return -1;

  if (cmd == NULL) return -1;

  /* 在PC控制和RC控制间切换. */
  if (CMD_KeyPressed(rc, CMD_KEY_SHIFT) && CMD_KeyPressed(rc, CMD_KEY_CTRL) &&
      CMD_KeyPressed(rc, CMD_KEY_Q))
    cmd->pc_ctrl = true;

  if (CMD_KeyPressed(rc, CMD_KEY_SHIFT) && CMD_KeyPressed(rc, CMD_KEY_CTRL) &&
      CMD_KeyPressed(rc, CMD_KEY_E))
    cmd->pc_ctrl = false;

  /* PC键位映射和逻辑. */
  if (cmd->pc_ctrl) {
    cmd->gimbal.delta_eulr.yaw = (float)rc->mouse.x * cmd->param->sens_mouse;
    cmd->gimbal.delta_eulr.pit = (float)rc->mouse.y * cmd->param->sens_mouse;

    if (rc->mouse.l_click) {
      if (rc->mouse.r_click) {
        cmd->shoot.shoot_freq_hz = 5u;
        cmd->shoot.bullet_speed = 20.0f;
      } else {
        cmd->shoot.shoot_freq_hz = 10u;
        cmd->shoot.bullet_speed = 10.0f;
      }
    } else {
      cmd->shoot.shoot_freq_hz = 0u;
      cmd->shoot.bullet_speed = 0.0f;
    }

    if (CMD_KeyPressed(rc, CMD_KEY_SHIFT) && CMD_KeyPressed(rc, CMD_KEY_CTRL)) {
      if (CMD_KeyPressed(rc, CMD_KEY_A))
        cmd->shoot.mode = SHOOT_MODE_SAFE;

      else if (CMD_KeyPressed(rc, CMD_KEY_S))
        cmd->shoot.mode = SHOOT_MODE_STDBY;

      else if (CMD_KeyPressed(rc, CMD_KEY_D))
        cmd->shoot.mode = SHOOT_MODE_FIRE;

      else
        cmd->shoot.mode = SHOOT_MODE_RELAX;
    }
  } else {
    /* RC键位映射和逻辑. */
    if ((rc->sw_l == CMD_SW_ERR) || (rc->sw_r == CMD_SW_ERR)) {
      cmd->chassis.mode = CHASSIS_MODE_RELAX;
      cmd->gimbal.mode = GIMBAL_MODE_RELAX;
      cmd->shoot.mode = SHOOT_MODE_RELAX;
    } else {
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
      switch (rc->sw_r) {
        case CMD_SW_UP:
          cmd->gimbal.mode = GIMBAL_MODE_FIX;
          cmd->shoot.mode = SHOOT_MODE_SAFE;
          cmd->shoot.shoot_freq_hz = 0.0f;
          cmd->shoot.bullet_speed = 0.0f;
          break;

        case CMD_SW_MID:
          cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;
          cmd->shoot.mode = SHOOT_MODE_STDBY;
          cmd->shoot.shoot_freq_hz = 0.0f;
          cmd->shoot.bullet_speed = 10.0f;
          break;

        case CMD_SW_DOWN:
          cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;
          cmd->shoot.mode = SHOOT_MODE_FIRE;
          cmd->shoot.shoot_freq_hz = 10u;
          cmd->shoot.bullet_speed = 10.0f;
          break;

        case CMD_SW_ERR:
          cmd->gimbal.mode = GIMBAL_MODE_RELAX;
          cmd->shoot.mode = SHOOT_MODE_RELAX;
      }
      cmd->chassis.ctrl_vec.vx = rc->ch_l_x;
      cmd->chassis.ctrl_vec.vy = rc->ch_l_y;
      cmd->gimbal.delta_eulr.yaw = rc->ch_r_x * cmd->param->sens_rc;
      cmd->gimbal.delta_eulr.pit = rc->ch_r_y * cmd->param->sens_rc;
    }
  }
  return 0;
}
