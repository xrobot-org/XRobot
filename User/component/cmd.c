/*
  控制命令
*/

#include "cmd.h"

/**
 * @brief 检查按键是否按下
 *
 * @param rc 遥控器数据
 * @param key 按键名称
 * @return true 按下
 * @return false 未按下
 */
static bool CMD_KeyPressedRc(const CMD_RC_t *rc, CMD_KeyValue_t key,
                             bool stateful) {
  if (key == CMD_L_CLICK) {
    return rc->mouse.l_click;
  }
  if (key == CMD_R_CLICK) {
    return rc->mouse.r_click;
  }
  bool current_key_stat = rc->key & (1u << key);
  if (stateful)
    return current_key_stat && !(rc->key_last & (1u << key));
  else
    return current_key_stat;
}

/**
 * @brief 行为转换为对应按键
 *
 * @param cmd 主结构体
 * @param behavior 行为
 * @return uint16_t 行为对应的按键
 */
static inline uint16_t CMD_BehaviorToKey(CMD_t *cmd, CMD_Behavior_t behavior) {
  return cmd->param->map.Key_Mapping[behavior];
}

/**
 * @brief 检查是否启用上位机控制指令覆盖
 *
 * @param cmd 主结构体
 * @return true 启用
 * @return false 不启用
 */
inline bool CMD_CheckHostOverwrite(CMD_t *cmd) { return cmd->host_overwrite; }

/**
 * @brief 初始化命令解析
 *
 * @param cmd 主结构体
 * @param param 参数
 * @return int8_t 0对应没有错误
 */
int8_t CMD_Init(CMD_t *cmd, const CMD_Params_t *param) {
  if (cmd == NULL) return -1;
  if (param == NULL) return -1;

  cmd->pc_ctrl = false;
  cmd->param = param;

  return 0;
}

/**
 * @brief rc失控时机器人恢复放松模式
 *
 * @param cmd 主结构体
 */
static void CMD_RcLostLogic(CMD_t *cmd) {
  cmd->chassis.mode = CHASSIS_MODE_RELAX;
  cmd->gimbal.mode = GIMBAL_MODE_RELAX;
  cmd->shoot.mode = SHOOT_MODE_RELAX;
}

/**
 * @brief 解析pc行为逻辑
 *
 * @param rc 遥控器数据
 * @param cmd 主结构体
 * @param dt_sec 两次解析的间隔
 */
static void CMD_PcLogic(CMD_RC_t *rc, CMD_t *cmd, float dt_sec) {
  cmd->gimbal.delta_eulr.yaw =
      (float)rc->mouse.x * dt_sec * cmd->param->sens_mouse;
  cmd->gimbal.delta_eulr.pit =
      (float)rc->mouse.y * dt_sec * cmd->param->sens_mouse;
  cmd->chassis.ctrl_vec.vx = cmd->chassis.ctrl_vec.vy = 0.0f;
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_FORE), false)) {
    cmd->chassis.ctrl_vec.vy += cmd->param->move.move_sense;
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_BACK), false)) {
    cmd->chassis.ctrl_vec.vy -= cmd->param->move.move_sense;
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_LEFT), false)) {
    cmd->chassis.ctrl_vec.vx += cmd->param->move.move_sense;
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_RIGHT), false)) {
    cmd->chassis.ctrl_vec.vx -= cmd->param->move.move_sense;
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_ACCELERATE),
                       false)) {
    cmd->chassis.ctrl_vec.vx *= cmd->param->move.move_fast_sense;
    cmd->chassis.ctrl_vec.vy *= cmd->param->move.move_fast_sense;
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_DECELEBRATE),
                       false)) {
    cmd->chassis.ctrl_vec.vx *= cmd->param->move.move_slow_sense;
    cmd->chassis.ctrl_vec.vy *= cmd->param->move.move_slow_sense;
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_FIRE), false)) {
    cmd->shoot.mode = SHOOT_MODE_FIRE;
    cmd->shoot.shoot_freq_hz = 10u;
    cmd->shoot.bullet_speed = 100.0f;
  } else {
    cmd->shoot.mode = SHOOT_MODE_STDBY;
    cmd->shoot.shoot_freq_hz = 0u;
    cmd->shoot.bullet_speed = 20.0f;
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_ROTOR), true)) {
    cmd->chassis.mode_rotor++;
    cmd->chassis.mode_rotor %= ROTOR_MODE_NUM;
    if (cmd->chassis.mode_rotor == ROTOR_MODE_NONE) {
      cmd->chassis.mode = CHASSIS_MODE_FOLLOW_GIMBAL;
    } else {
      cmd->chassis.mode = CHASSIS_MODE_ROTOR;
    }
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_OPENCOVER),
                       true)) {
    cmd->shoot.cover_open = !cmd->shoot.cover_open;
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_BUFF), true)) {
    if (cmd->ai_status == AI_STATUS_HITSWITCH) {
      CMD_RefereeAdd(&(cmd->referee), CMD_UI_HIT_SWITCH_STOP);
      cmd->host_overwrite = false;
      cmd->ai_status = AI_STATUS_STOP;
    } else if (cmd->ai_status == AI_STATUS_AUTOAIM) {
      // TODO: 提醒操作员
    } else {
      CMD_RefereeAdd(&(cmd->referee), CMD_UI_HIT_SWITCH_START);
      cmd->ai_status = AI_STATUS_HITSWITCH;
      cmd->host_overwrite = true;
    }
  }
  if (CMD_KeyPressedRc(rc, CMD_BehaviorToKey(cmd, CMD_BEHAVIOR_AUTOAIM),
                       true)) {
    if (cmd->ai_status == AI_STATUS_AUTOAIM) {
      cmd->host_overwrite = false;
      cmd->ai_status = AI_STATUS_STOP;
      CMD_RefereeAdd(&(cmd->referee), CMD_UI_AUTO_AIM_STOP);
    } else {
      cmd->ai_status = AI_STATUS_AUTOAIM;
      cmd->host_overwrite = true;
      CMD_RefereeAdd(&(cmd->referee), CMD_UI_AUTO_AIM_START);
    }
  } else {
    cmd->host_overwrite = false;
    // TODO: 修复逻辑
  }
  rc->key_last = rc->key;
}

/**
 * @brief 解析rc行为逻辑
 *
 * @param rc 遥控器数据
 * @param cmd 主结构体
 * @param dt_sec 两次解析的间隔
 */
static void CMD_RcLogic(const CMD_RC_t *rc, CMD_t *cmd, float dt_sec) {
  switch (rc->sw_l) {
    case CMD_SW_UP:
      cmd->chassis.mode = CHASSIS_MODE_BREAK;
      break;

    case CMD_SW_MID:
      cmd->chassis.mode = CHASSIS_MODE_FOLLOW_GIMBAL;
      break;

    case CMD_SW_DOWN:
      cmd->chassis.mode = CHASSIS_MODE_ROTOR;
      cmd->chassis.mode_rotor = ROTOR_MODE_CW;
      break;

    case CMD_SW_ERR:
      cmd->chassis.mode = CHASSIS_MODE_RELAX;
      break;
  }
  switch (rc->sw_r) {
    case CMD_SW_UP:
      cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;
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
  cmd->gimbal.delta_eulr.yaw = rc->ch_r_x * dt_sec * cmd->param->sens_rc;
  cmd->gimbal.delta_eulr.pit = rc->ch_r_y * dt_sec * cmd->param->sens_rc;
}

/**
 * @brief 解析上位机命令
 *
 * @param host host数据
 * @param cmd 命令
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t CMD_ParseHost(const CMD_Host_t *host, CMD_t *cmd, float dt_sec) {
  (void)dt_sec;
  if (host == NULL) return -1;
  if (cmd == NULL) return -1;

  cmd->gimbal.delta_eulr.yaw = host->gimbal_delta.yaw;
  cmd->gimbal.delta_eulr.pit = host->gimbal_delta.pit;

  if (host->fire) {
    cmd->shoot.shoot_freq_hz = 10u;
    cmd->shoot.bullet_speed = 10.0f;
  } else {
    cmd->shoot.shoot_freq_hz = 0u;
    cmd->shoot.bullet_speed = 0.0f;
  }
  return 0;
}

/**
 * @brief 解析命令
 *
 * @param rc 遥控器数据
 * @param cmd 命令
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t CMD_ParseRc(CMD_RC_t *rc, CMD_t *cmd, float dt_sec) {
  if (rc == NULL) return -1;
  if (cmd == NULL) return -1;

  /* 在PC控制和RC控制间切换 */
  if (CMD_KeyPressedRc(rc, CMD_KEY_SHIFT, false) &&
      CMD_KeyPressedRc(rc, CMD_KEY_CTRL, false) &&
      CMD_KeyPressedRc(rc, CMD_KEY_Q, false))
    cmd->pc_ctrl = true;

  if (CMD_KeyPressedRc(rc, CMD_KEY_SHIFT, false) &&
      CMD_KeyPressedRc(rc, CMD_KEY_CTRL, false) &&
      CMD_KeyPressedRc(rc, CMD_KEY_E, false))
    cmd->pc_ctrl = false;

  if ((rc->sw_l == CMD_SW_ERR) || (rc->sw_r == CMD_SW_ERR)) {
    CMD_RcLostLogic(cmd);
  } else {
    if (cmd->pc_ctrl) {
      CMD_PcLogic(rc, cmd, dt_sec);
    } else {
      CMD_RcLogic(rc, cmd, dt_sec);
    }
  }
  return 0;
}

/**
 * @brief 添加向Referee发送的命令
 *
 * @param ref 命令队列
 * @param cmd 要添加的命令
 * @return int8_t 0对应没有错误
 */
int8_t CMD_RefereeAdd(CMD_RefereeCmd_t *ref, CMD_UI_t cmd) {
  if (ref->counter >= CMD_REFEREE_MAX_NUM || ref->counter < 0) return -1;
  ref->cmd[ref->counter] = cmd;
  ref->counter++;
  return 0;
}
