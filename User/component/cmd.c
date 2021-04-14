/*
  控制命令
*/

#include "cmd.h"

#include <string.h>

/**
 * @brief 行为转换为对应按键
 *
 * @param cmd 主结构体
 * @param behavior 行为
 * @return uint16_t 行为对应的按键
 */
static inline CMD_KeyValue_t CMD_BehaviorToKey(CMD_t *cmd,
                                               CMD_Behavior_t behavior) {
  return cmd->param->map.key_map[behavior].key;
}

static inline CMD_ActiveType_t CMD_BehaviorToActive(CMD_t *cmd,
                                                    CMD_Behavior_t behavior) {
  return cmd->param->map.key_map[behavior].active;
}

/**
 * @brief 检查按键是否按下
 *
 * @param rc 遥控器数据
 * @param key 按键名称
 * @param stateful 是否为状态切换按键
 * @return true 按下
 * @return false 未按下
 */
static bool CMD_KeyPressedRc(const CMD_RC_t *rc, CMD_KeyValue_t key) {
  /* 按下按键为鼠标左、右键 */
  if (key == CMD_KEY_L_CLICK) {
    return rc->mouse.click.l;
  }
  if (key == CMD_KEY_R_CLICK) {
    return rc->mouse.click.r;
  }
  return rc->key & (1u << key);
}

static bool CMD_BehaviorOccurredRc(const CMD_RC_t *rc, CMD_t *cmd,
                                   CMD_Behavior_t behavior) {
  CMD_KeyValue_t key = CMD_BehaviorToKey(cmd, behavior);
  CMD_ActiveType_t active = CMD_BehaviorToActive(cmd, behavior);

  bool now_key_pressed, last_key_pressed;

  /* 按下按键为鼠标左、右键 */
  if (key == CMD_KEY_L_CLICK) {
    now_key_pressed = rc->mouse.click.l;
    last_key_pressed = cmd->mouse_last.click.l;
  } else if (key == CMD_KEY_R_CLICK) {
    now_key_pressed = rc->mouse.click.r;
    last_key_pressed = cmd->mouse_last.click.r;
  } else {
    now_key_pressed = rc->key & (1u << key);
    last_key_pressed = cmd->key_last & (1u << key);
  }

  switch (active) {
    case CMD_ACTIVE_PRESSING:
      return now_key_pressed && !last_key_pressed;
    case CMD_ACTIVE_RASING:
      return !now_key_pressed && last_key_pressed;
    case CMD_ACTIVE_PRESSED:
      return now_key_pressed;
  }
}

/**
 * @brief 解析pc行为逻辑
 *
 * @param rc 遥控器数据
 * @param cmd 主结构体
 * @param dt_sec 两次解析的间隔
 */
static void CMD_PcLogic(const CMD_RC_t *rc, CMD_t *cmd, float dt_sec) {
  cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;

  /* 云台设置为鼠标控制欧拉角的变化，底盘的控制向量设置为零 */
  cmd->gimbal.delta_eulr.yaw =
      (float)rc->mouse.x * dt_sec * cmd->param->sens_mouse;
  cmd->gimbal.delta_eulr.pit =
      (float)(-rc->mouse.y) * dt_sec * cmd->param->sens_mouse;
  cmd->chassis.ctrl_vec.vx = cmd->chassis.ctrl_vec.vy = 0.0f;
  cmd->shoot.reverse_trig = false;

  /* 按键行为映射相关逻辑 */
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_FORE)) {
    cmd->chassis.ctrl_vec.vy += cmd->param->move.move_norm_sense;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_BACK)) {
    cmd->chassis.ctrl_vec.vy -= cmd->param->move.move_norm_sense;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_LEFT)) {
    cmd->chassis.ctrl_vec.vx -= cmd->param->move.move_norm_sense;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_RIGHT)) {
    cmd->chassis.ctrl_vec.vx += cmd->param->move.move_norm_sense;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_ACCELERATE)) {
    cmd->chassis.ctrl_vec.vx *= cmd->param->move.move_fast_sense;
    cmd->chassis.ctrl_vec.vy *= cmd->param->move.move_fast_sense;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_DECELEBRATE)) {
    cmd->chassis.ctrl_vec.vx *= cmd->param->move.move_slow_sense;
    cmd->chassis.ctrl_vec.vy *= cmd->param->move.move_slow_sense;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_FIRE)) {
    /* 切换至开火模式，设置相应的射击频率和弹丸初速度 */
    cmd->shoot.mode = SHOOT_MODE_LOADED;
    cmd->shoot.fire = true;
  } else {
    /* 切换至准备模式，停止射击 */
    cmd->shoot.mode = SHOOT_MODE_LOADED;
    cmd->shoot.fire = false;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_FIRE_MODE)) {
    /* 每按一次依次切换开火下一个模式 */
    cmd->shoot.fire_mode++;
    cmd->shoot.fire_mode %= FIRE_MODE_NUM;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_ROTOR)) {
    /* 切换到小陀螺模式 */
    cmd->chassis.mode = CHASSIS_MODE_ROTOR;
    cmd->chassis.mode_rotor = ROTOR_MODE_RAND;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_OPENCOVER)) {
    /* 每按一次开、关弹舱盖 */
    cmd->shoot.cover_open = !cmd->shoot.cover_open;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_BUFF)) {
    if (cmd->ai_status == AI_STATUS_HITSWITCH) {
      /* 停止ai的打符模式，停用host控制 */
      CMD_RefereeAdd(&(cmd->referee), CMD_UI_HIT_SWITCH_STOP);
      cmd->host_overwrite = false;
      cmd->ai_status = AI_STATUS_STOP;
    } else if (cmd->ai_status == AI_STATUS_AUTOAIM) {
      /* 自瞄模式中切换失败提醒 */
    } else {
      /* ai切换至打符模式，启用host控制 */
      CMD_RefereeAdd(&(cmd->referee), CMD_UI_HIT_SWITCH_START);
      cmd->ai_status = AI_STATUS_HITSWITCH;
      cmd->host_overwrite = true;
    }
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_AUTOAIM)) {
    if (cmd->ai_status == AI_STATUS_AUTOAIM) {
      /* 停止ai的自瞄模式，停用host控制 */
      cmd->host_overwrite = false;
      cmd->ai_status = AI_STATUS_STOP;
      CMD_RefereeAdd(&(cmd->referee), CMD_UI_AUTO_AIM_STOP);
    } else {
      /* ai切换至自瞄模式，启用host控制 */
      cmd->ai_status = AI_STATUS_AUTOAIM;
      cmd->host_overwrite = true;
      CMD_RefereeAdd(&(cmd->referee), CMD_UI_AUTO_AIM_START);
    }
  } else {
    cmd->host_overwrite = false;
    // TODO: 修复逻辑
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_REVTRIG)) {
    /* 按下拨弹反转 */
    cmd->shoot.reverse_trig = true;
  }
  if (CMD_BehaviorOccurredRc(rc, cmd, CMD_BEHAVIOR_FOLLOWGIMBAL35)) {
    cmd->chassis.mode = CHASSIS_MODE_FOLLOW_GIMBAL;
  }
  /* 保存当前按下的键位状态 */
  cmd->key_last = rc->key;
  memcpy(&(cmd->mouse_last), &(rc->mouse), sizeof(cmd->mouse_last));
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
      /* 左拨杆相应行为选择和解析 */
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
      /* 右拨杆相应行为选择和解析 */
    case CMD_SW_UP:
      cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;
      cmd->shoot.mode = SHOOT_MODE_SAFE;
      break;

    case CMD_SW_MID:
      cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;
      cmd->shoot.fire = false;
      cmd->shoot.mode = SHOOT_MODE_LOADED;
      break;

    case CMD_SW_DOWN:
      cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;
      cmd->shoot.mode = SHOOT_MODE_LOADED;
      cmd->shoot.fire_mode = FIRE_MODE_CONT;
      cmd->shoot.fire = true;
      break;

    case CMD_SW_ERR:
      cmd->gimbal.mode = GIMBAL_MODE_RELAX;
      cmd->shoot.mode = SHOOT_MODE_RELAX;
  }
  /* 将操纵杆的对应值转换为底盘的控制向量和云台变化的欧拉角 */
  cmd->chassis.ctrl_vec.vx = rc->ch.l.x;
  cmd->chassis.ctrl_vec.vy = rc->ch.l.y;
  cmd->gimbal.delta_eulr.yaw = rc->ch.r.x * dt_sec * cmd->param->sens_stick;
  cmd->gimbal.delta_eulr.pit = rc->ch.r.y * dt_sec * cmd->param->sens_stick;
}

/**
 * @brief rc失控时机器人恢复放松模式
 *
 * @param cmd 主结构体
 */
static void CMD_RcLostLogic(CMD_t *cmd) {
  /* 机器人底盘、云台、发射器运行模式恢复至放松模式 */
  cmd->chassis.mode = CHASSIS_MODE_RELAX;
  cmd->gimbal.mode = GIMBAL_MODE_RELAX;
  cmd->shoot.mode = SHOOT_MODE_RELAX;
}

/**
 * @brief 初始化命令解析
 *
 * @param cmd 主结构体
 * @param param 参数
 * @return int8_t 0对应没有错误
 */
int8_t CMD_Init(CMD_t *cmd, const CMD_Params_t *param) {
  /* 指针检测 */
  if (cmd == NULL) return -1;
  if (param == NULL) return -1;

  /* 设置机器人的命令参数，初始化控制方式为rc控制 */
  cmd->pc_ctrl = false;
  cmd->param = param;

  return 0;
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
 * @brief 解析命令
 *
 * @param rc 遥控器数据
 * @param cmd 命令
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t CMD_ParseRc(CMD_RC_t *rc, CMD_t *cmd, float dt_sec) {
  /* 指针检测 */
  if (rc == NULL) return -1;
  if (cmd == NULL) return -1;

  /* 在pc控制和rc控制间切换 */
  if (CMD_KeyPressedRc(rc, CMD_KEY_SHIFT) &&
      CMD_KeyPressedRc(rc, CMD_KEY_CTRL) && CMD_KeyPressedRc(rc, CMD_KEY_Q))
    cmd->pc_ctrl = true;

  if (CMD_KeyPressedRc(rc, CMD_KEY_SHIFT) &&
      CMD_KeyPressedRc(rc, CMD_KEY_CTRL) && CMD_KeyPressedRc(rc, CMD_KEY_E))
    cmd->pc_ctrl = false;
  /*c当rc丢控时，恢复机器人至默认状态 */
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
 * @brief 解析上位机命令
 *
 * @param host host数据
 * @param cmd 命令
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t CMD_ParseHost(const CMD_Host_t *host, CMD_t *cmd, float dt_sec) {
  (void)dt_sec; /* 未使用dt_sec，消除警告 */
  /* 指针检测 */
  if (host == NULL) return -1;
  if (cmd == NULL) return -1;

  /* 云台欧拉角设置为host相应的变化的欧拉角 */
  cmd->gimbal.delta_eulr.yaw = host->gimbal_delta.yaw;
  cmd->gimbal.delta_eulr.pit = host->gimbal_delta.pit;

  /* host射击命令，设置不同的射击频率和弹丸初速度 */
  if (host->fire) {
    cmd->shoot.mode = SHOOT_MODE_LOADED;
    cmd->shoot.fire = true;
  } else {
    cmd->shoot.mode = SHOOT_MODE_SAFE;
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
  /* 指针检测 */
  if (ref == NULL) return -1;
  /* 越界检测 */
  if (ref->counter >= CMD_REFEREE_MAX_NUM || ref->counter < 0) return -1;

  /* 添加机器人当前行为状态到画图的命令队列中 */
  ref->cmd[ref->counter] = cmd;
  ref->counter++;
  return 0;
}
