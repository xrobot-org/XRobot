/**
 * @file cmd.h
 * @author Qu Shen
 * @brief
 * @version 1.0.0
 * @date 2021-04-23
 *
 * @copyright Copyright (c) 2021
 *
 * 控制来源(CtrlSource)有两个: 遥控(RC) & 上位机(Host)
 *
 * 遥控又分为两个控制方式(CtrlMethod):
 *     摇杆拨杆控制(Joystick & Switch) & 键盘鼠标控制(Mouse & Keyboard)
 *
 * RC -> Joystick Switch logic -> CMD
 *              or
 * RC -> Mouse keyboard logic -> CMD
 *
 * 上位机控制不区分控制方式
 * Host -> ParseHsot -> CMD
 *
 */

#include "cmd.h"

#include <string.h>

/**
 * @brief 找到行为对应的按键值
 *
 * @param cmd 控制指令数据
 * @param behavior 行为
 * @return CMD_KeyValue_t 按键值
 */
static inline CMD_KeyValue_t CMD_BehaviorToKey(const CMD_t *cmd,
                                               CMD_Behavior_t behavior) {
  return cmd->param->key_map[behavior].key;
}

/**
 * @brief 找到行为对应的出发类型
 *
 * @param cmd 控制指令数据
 * @param behavior 行为
 * @return CMD_ActiveType_t 触发类型
 */
static inline CMD_ActiveType_t CMD_BehaviorToActive(const CMD_t *cmd,
                                                    CMD_Behavior_t behavior) {
  return cmd->param->key_map[behavior].active;
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
static bool CMD_KeyPressed(const CMD_RC_t *rc, CMD_KeyValue_t key) {
  /* 鼠标左右键需要单独判断 */
  switch (key) {
    case CMD_KEY_L_CLICK:
      return rc->mouse.click.l;

    case CMD_KEY_R_CLICK:
      return rc->mouse.click.r;

    default:
      return rc->key & (1u << key);
  }
}

/**
 * @brief 检查行为触发条件是否满足
 *
 * @param rc 遥控器数据
 * @param cmd 控制指令数据
 * @param behavior 行为
 * @return true 满足
 * @return false 不满足
 */
static bool CMD_BehaviorOccurred(const CMD_RC_t *rc, const CMD_t *cmd,
                                 CMD_Behavior_t behavior) {
  CMD_KeyValue_t key = CMD_BehaviorToKey(cmd, behavior);
  CMD_ActiveType_t active = CMD_BehaviorToActive(cmd, behavior);

  bool now_key_pressed, last_key_pressed;
  /* 鼠标左右键需要单独判断 */
  switch (key) {
    case CMD_KEY_L_CLICK:
      now_key_pressed = rc->mouse.click.l;
      last_key_pressed = cmd->mouse_last.click.l;
      break;

    case CMD_KEY_R_CLICK:
      now_key_pressed = rc->mouse.click.r;
      last_key_pressed = cmd->mouse_last.click.r;
      break;

    default:
      now_key_pressed = rc->key & (1u << key);
      last_key_pressed = cmd->key_last & (1u << key);
      break;
  }
  switch (active) {
    case CMD_ACTIVE_PRESSING:
      return now_key_pressed && !last_key_pressed;
    case CMD_ACTIVE_RELEASING:
      return !now_key_pressed && last_key_pressed;
    case CMD_ACTIVE_PRESSED:
      return now_key_pressed;
    default:
      return false;
  }
}

/**
 * @brief 解析键盘鼠标控制逻辑
 *
 * @param rc 遥控器数据
 * @param cmd 控制指令数据
 * @param dt_sec 两次解析的间隔
 */
static void CMD_MouseKeyboardLogic(const CMD_RC_t *rc, CMD_t *cmd,
                                   float dt_sec) {
  cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;

  /* 云台设置为鼠标控制欧拉角的变化，底盘的控制向量设置为零 */
  cmd->gimbal.delta_eulr.yaw =
      (float)rc->mouse.x * dt_sec * cmd->param->sens_mouse;
  cmd->gimbal.delta_eulr.pit =
      (float)(-rc->mouse.y) * dt_sec * cmd->param->sens_mouse;
  cmd->chassis.ctrl_vec.vx = cmd->chassis.ctrl_vec.vy = 0.0f;
  cmd->launcher.reverse_trig = false;

  /* 按键行为映射相关逻辑 */
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_FORE)) {
    cmd->chassis.ctrl_vec.vy += cmd->param->move.sense_norm;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_BACK)) {
    cmd->chassis.ctrl_vec.vy -= cmd->param->move.sense_norm;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_LEFT)) {
    cmd->chassis.ctrl_vec.vx -= cmd->param->move.sense_norm;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_RIGHT)) {
    cmd->chassis.ctrl_vec.vx += cmd->param->move.sense_norm;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_ACCELERATE)) {
    cmd->chassis.ctrl_vec.vx *= cmd->param->move.sense_fast;
    cmd->chassis.ctrl_vec.vy *= cmd->param->move.sense_fast;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_DECELEBRATE)) {
    cmd->chassis.ctrl_vec.vx *= cmd->param->move.sense_slow;
    cmd->chassis.ctrl_vec.vy *= cmd->param->move.sense_slow;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_FIRE)) {
    /* 切换至开火模式，设置相应的发射频率和弹丸初速度 */
    cmd->launcher.mode = LAUNCHER_MODE_LOADED;
    cmd->launcher.fire = true;
  } else {
    /* 切换至准备模式，停止发射 */
    cmd->launcher.mode = LAUNCHER_MODE_LOADED;
    cmd->launcher.fire = false;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_FIRE_MODE)) {
    /* 每按一次依次切换开火下一个模式 */
    cmd->launcher.fire_mode++;
    cmd->launcher.fire_mode %= FIRE_MODE_NUM;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_ROTOR)) {
    /* 切换到小陀螺模式 */
    cmd->chassis.mode = CHASSIS_MODE_ROTOR;
    cmd->chassis.mode_rotor = ROTOR_MODE_RAND;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_OPENCOVER)) {
    /* 每按一次开、关弹舱盖 */
    cmd->launcher.cover_open = !cmd->launcher.cover_open;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_BUFF)) {
    if (cmd->ai_mode == AI_MODE_HITBUFF) {
      /* 停止ai的打符模式，停用host控制 */
      cmd->ctrl_source = CMD_SOURCE_RC;
      cmd->ai_mode = AI_MODE_STOP;
    } else if (cmd->ai_mode == AI_MODE_AUTOAIM) {
      /* 自瞄模式中切换失败提醒 */
    } else {
      /* ai切换至打符模式，启用host控制 */
      cmd->ai_mode = AI_MODE_HITBUFF;
      cmd->ctrl_source = CMD_SOURCE_HOST;
    }
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_AUTOAIM)) {
    if (cmd->ai_mode == AI_MODE_AUTOAIM) {
      /* 停止ai的自瞄模式，停用host控制 */
      cmd->ctrl_source = CMD_SOURCE_RC;
      cmd->ai_mode = AI_MODE_STOP;
    } else {
      /* ai切换至自瞄模式，启用host控制 */
      cmd->ai_mode = AI_MODE_AUTOAIM;
      cmd->ctrl_source = CMD_SOURCE_HOST;
    }
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_REVTRIG)) {
    /* 按下拨弹反转 */
    cmd->launcher.reverse_trig = true;
  }
  if (CMD_BehaviorOccurred(rc, cmd, CMD_BEHAVIOR_FOLLOWGIMBAL35)) {
    cmd->chassis.mode = CHASSIS_MODE_FOLLOW_GIMBAL;
  }
  /* 保存当前按下的键位状态 */
  cmd->key_last = rc->key;
  memcpy(&(cmd->mouse_last), &(rc->mouse), sizeof(cmd->mouse_last));
}

/**
 * @brief 解析摇杆拨杆控制逻辑
 *
 * @param rc 遥控器数据
 * @param cmd 控制指令数据
 * @param dt_sec 两次解析的间隔
 */
static void CMD_JoystickSwitchLogic(const CMD_RC_t *rc, CMD_t *cmd,
                                    float dt_sec) {
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
      cmd->launcher.mode = LAUNCHER_MODE_SAFE;
      break;

    case CMD_SW_MID:
      cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;
      cmd->launcher.fire = false;
      cmd->launcher.mode = LAUNCHER_MODE_LOADED;
      break;

    case CMD_SW_DOWN:
      cmd->gimbal.mode = GIMBAL_MODE_ABSOLUTE;
      cmd->launcher.mode = LAUNCHER_MODE_LOADED;
      cmd->launcher.fire_mode = FIRE_MODE_CONT;
      cmd->launcher.fire = true;
      break;

    case CMD_SW_ERR:
      cmd->gimbal.mode = GIMBAL_MODE_RELAX;
      cmd->launcher.mode = LAUNCHER_MODE_RELAX;
  }
  /* 将操纵杆的对应值转换为底盘的控制向量和云台变化的欧拉角 */
  cmd->chassis.ctrl_vec.vx = rc->ch.l.x;
  cmd->chassis.ctrl_vec.vy = rc->ch.l.y;
  cmd->gimbal.delta_eulr.yaw = rc->ch.r.x * dt_sec * cmd->param->sens_stick;
  cmd->gimbal.delta_eulr.pit = rc->ch.r.y * dt_sec * cmd->param->sens_stick;
}

static bool CMD_CheckRcLost(const CMD_RC_t *rc) {
  return (rc->sw_l == CMD_SW_ERR) || (rc->sw_r == CMD_SW_ERR);
}

/**
 * @brief rc失控时机器人恢复放松模式
 *
 * @param cmd 控制指令数据
 */
static void CMD_RcLostLogic(CMD_t *cmd) {
  /* 机器人底盘、云台、发射器运行模式恢复至放松模式 */
  cmd->chassis.mode = CHASSIS_MODE_RELAX;
  cmd->gimbal.mode = GIMBAL_MODE_RELAX;
  cmd->launcher.mode = LAUNCHER_MODE_RELAX;
}

/**
 * @brief 初始化命令解析
 *
 * @param cmd 控制指令数据
 * @param param 参数
 * @return int8_t 0对应没有错误
 */
int8_t CMD_Init(CMD_t *cmd, const CMD_Params_t *param) {
  /* 指针检测 */
  ASSERT(cmd);
  ASSERT(param);

  /* 设置机器人的命令参数，初始化控制方式为摇杆控制 */
  cmd->ctrl_method = CMD_METHOD_JOYSTICK_SWITCH;
  cmd->param = param;

  return 0;
}

/**
 * @brief 检查是否启用上位机控制指令覆盖
 *
 * @param cmd 控制指令数据
 * @return true 启用
 * @return false 不启用
 */
inline bool CMD_CheckHostOverwrite(CMD_t *cmd) {
  return cmd->ctrl_source == CMD_SOURCE_HOST;
}

/**
 * @brief 解析命令
 *
 * @param rc 遥控器数据
 * @param cmd 控制指令数据
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t CMD_ParseRc(const CMD_RC_t *rc, CMD_t *cmd, float dt_sec) {
  /* 指针检测 */
  ASSERT(rc);
  ASSERT(cmd);

  /* 在键盘鼠标和摇杆拨杆控制间切换 */
  if (CMD_KeyPressed(rc, CMD_KEY_SHIFT) && CMD_KeyPressed(rc, CMD_KEY_CTRL) &&
      CMD_KeyPressed(rc, CMD_KEY_Q)) {
    cmd->ctrl_method = CMD_METHOD_MOUSE_KEYBOARD;
  }

  if (CMD_KeyPressed(rc, CMD_KEY_SHIFT) && CMD_KeyPressed(rc, CMD_KEY_CTRL) &&
      CMD_KeyPressed(rc, CMD_KEY_E)) {
    cmd->ctrl_method = CMD_METHOD_JOYSTICK_SWITCH;
  }

  /* 当遥控链路丢失时，恢复机器人至默认状态 */
  if (CMD_CheckRcLost(rc)) {
    /* 遥控链路应该拥有最高控制权，
     * 任何时候关闭遥控器，都必须保证机器人进入放松状态
     * 进而保证任何失控可以通过关闭遥控器来解决
     */
    CMD_RcLostLogic(cmd);
  } else {
    switch (cmd->ctrl_method) {
      case CMD_METHOD_MOUSE_KEYBOARD:
        CMD_MouseKeyboardLogic(rc, cmd, dt_sec);
        break;
      default:
        CMD_JoystickSwitchLogic(rc, cmd, dt_sec);
        break;
    }
  }
  return 0;
}

/**
 * @brief 解析上位机命令
 *
 * @param host host数据
 * @param cmd 控制指令数据
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t CMD_ParseHost(const CMD_Host_t *host, CMD_t *cmd, float dt_sec) {
  UNUSED(dt_sec); /* 未使用dt_sec，消除警告 */
  /* 指针检测 */
  ASSERT(host);
  ASSERT(cmd);

  /* 云台欧拉角设置为host相应的变化的欧拉角 */
  cmd->gimbal.delta_eulr.yaw = host->gimbal_delta.yaw;
  cmd->gimbal.delta_eulr.pit = host->gimbal_delta.pit;

  /* host发射命令，设置不同的发射频率和弹丸初速度 */
  if (cmd->ai_mode == AI_MODE_HITBUFF) {
    if (host->fire) {
      cmd->launcher.mode = LAUNCHER_MODE_LOADED;
      cmd->launcher.fire = true;
    } else {
      cmd->launcher.mode = LAUNCHER_MODE_SAFE;
    }
  }
  return 0;
}

/**
 * @brief 导出控制指令UI数据
 *
 * @param cmd_ui 控制指令UI数据
 * @param cmd 控制指令数据
 */
void CMD_PackUi(CMD_UI_t *cmd_ui, const CMD_t *cmd) {
  cmd_ui->ctrl_method = cmd->ctrl_method;
  cmd_ui->ctrl_source = cmd->ctrl_source;
}
