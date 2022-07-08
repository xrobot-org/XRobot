/**
 * @file cmd->h
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

#include "comp_cmd.hpp"

#include <string.h>

#include "message.hpp"
#include "om.h"

using namespace Component;

CMD::CMD(CMD::Param& param)
    : param_(param),
      ctrl_source_(CMD_SOURCE_RC),
      ctrl_method_(CMD_METHOD_JOYSTICK_SWITCH) {
  memset(&this->rc_, 0, sizeof(this->rc_));
  memset(&this->gimbal_, 0, sizeof(this->gimbal_));
  memset(&this->chassis_, 0, sizeof(this->chassis_));
  memset(&this->launcher_, 0, sizeof(this->launcher_));

  auto cmd_thread = [](void* arg) {
    CMD* cmd = (CMD*)arg;

    DECLARE_TOPIC(ch_tp, cmd->chassis_, "cmd_chassis", true);
    DECLARE_TOPIC(gm_tp, cmd->gimbal_, "cmd_gimbal", true);
    DECLARE_TOPIC(la_tp, cmd->launcher_, "cmd_launcher", true);
    DECLARE_TOPIC(ui_tp, cmd->ui_, "cmd_ui", true);

    DECLARE_SUBER(rc_sub, cmd->rc_, "cmd_rc");
    DECLARE_SUBER(host_sub, cmd->host_, "cmd_host");

    while (1) {
      /* 将接收机数据解析为指令数据 */
      rc_sub.DumpData();

      cmd->PraseRC();

      /* 判断是否需要让上位机覆写指令 */
      if (cmd->HostCtrl()) {
        if (host_sub.DumpData()) cmd->PraseHost();
      }

      cmd->PackUI();

      ch_tp.Publish();
      gm_tp.Publish();
      la_tp.Publish();
      ui_tp.Publish();

      /* 运行结束，等待下一次唤醒 */
      cmd->thread_.Sleep(2);
    }
  };

  THREAD_DECLEAR(this->thread_, cmd_thread, 256, System::Thread::High, this);
}

bool CMD::HostCtrl() {
#if HOST_CTRL_PRIORITY
  /* 保证丢控时上位机无法控制 */
  return !this->RCLost();
#else
  return this->ctrl_source_ == CMD_SOURCE_HOST;
#endif
}

bool CMD::PraseRC() {
  this->last_online_time_ = this->now_;
  this->now_ = System::Thread::GetTick();
  this->dt_ = (now_ - last_online_time_) / 1000.0f;

  /* 在键盘鼠标和摇杆拨杆控制间切换 */
  if (this->PraseMouseKey(CMD_KEY_SHIFT) && this->PraseMouseKey(CMD_KEY_CTRL) &&
      this->PraseMouseKey(CMD_KEY_Q)) {
    this->ctrl_method_ = CMD_METHOD_MOUSE_KEYBOARD;
  }

  if (this->PraseMouseKey(CMD_KEY_SHIFT) && this->PraseMouseKey(CMD_KEY_CTRL) &&
      this->PraseMouseKey(CMD_KEY_E)) {
    this->ctrl_method_ = CMD_METHOD_JOYSTICK_SWITCH;
  }

  /* 当遥控链路丢失时，恢复机器人至默认状态 */
  if (this->RCLost()) {
    /* 遥控链路应该拥有最高控制权，
     * 任何时候关闭遥控器，都必须保证机器人进入放松状态
     * 进而保证任何失控可以通过关闭遥控器来解决
     */
    this->StopCtrl();
  } else {
    switch (this->ctrl_method_) {
      case CMD_METHOD_MOUSE_KEYBOARD:
        this->PraseKeyboard();
        break;
      default:
        this->PraseJoystick();
        break;
    }
  }

  return true;
}

bool CMD::PraseHost() {
  /* 哨兵开启云台绝对角度控制 */
#if ID_SENTRY
  this->gimbal_.mode = GIMBAL_MODE_ABSOLUTE;
#endif

  /* 云台欧拉角设置为host相应的变化的欧拉角 */
  this->gimbal_.delta_eulr.yaw = this->host_.gimbal_delta.yaw;
  this->gimbal_.delta_eulr.pit = this->host_.gimbal_delta.pit;

  return true;
}

bool CMD::PraseMouseKey(CMD::Key key) {
  switch (key) {
    case CMD::CMD_KEY_L_CLICK:
      return this->rc_.mouse.click.l;

    case CMD::CMD_KEY_R_CLICK:
      return this->rc_.mouse.click.r;

    default:
      return this->rc_.key & (1u << key);
  }
}

bool CMD::PraseKeyboard() {
  this->gimbal_.mode = GIMBAL_MODE_ABSOLUTE;

  /* 云台设置为鼠标控制欧拉角的变化，底盘的控制向量设置为零 */
  this->gimbal_.delta_eulr.yaw =
      (float)this->rc_.mouse.x * this->dt_ * this->param_.sens_mouse;
  this->gimbal_.delta_eulr.pit =
      (float)(-this->rc_.mouse.y) * this->dt_ * this->param_.sens_mouse;
  this->chassis_.ctrl_vec.vx = this->chassis_.ctrl_vec.vy = 0.0f;
  this->launcher_.reverse_trig = false;

  /* 按键行为映射相关逻辑 */
  if (this->BehaviorOccurred(CMD_BEHAVIOR_FORE)) {
    this->chassis_.ctrl_vec.vy += this->param_.move.sense_norm;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_BACK)) {
    this->chassis_.ctrl_vec.vy -= this->param_.move.sense_norm;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_LEFT)) {
    this->chassis_.ctrl_vec.vx -= this->param_.move.sense_norm;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_RIGHT)) {
    this->chassis_.ctrl_vec.vx += this->param_.move.sense_norm;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_ACCELERATE)) {
    this->chassis_.ctrl_vec.vx *= this->param_.move.sense_fast;
    this->chassis_.ctrl_vec.vy *= this->param_.move.sense_fast;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_DECELEBRATE)) {
    this->chassis_.ctrl_vec.vx *= this->param_.move.sense_slow;
    this->chassis_.ctrl_vec.vy *= this->param_.move.sense_slow;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_FIRE)) {
    /* 切换至开火模式，设置相应的发射频率和弹丸初速度 */
    this->launcher_.mode = LAUNCHER_MODE_LOADED;
    this->launcher_.fire = true;
  } else {
    /* 切换至准备模式，停止发射 */
    this->launcher_.mode = LAUNCHER_MODE_LOADED;
    this->launcher_.fire = false;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_FIRE_MODE)) {
    /* 每按一次依次切换开火下一个模式 */
    switch (this->launcher_.fire_mode) {
      case FIRE_MODE_SINGLE:
        this->launcher_.fire_mode = FIRE_MODE_BURST;
        break;
      case FIRE_MODE_BURST:
        this->launcher_.fire_mode = FIRE_MODE_CONT;
        break;
      case FIRE_MODE_CONT:
        this->launcher_.fire_mode = FIRE_MODE_SINGLE;
        break;
      default:
        break;
    }
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_ROTOR)) {
    /* 切换到小陀螺模式 */
    this->chassis_.mode = CHASSIS_MODE_ROTOR;
    this->chassis_.mode_rotor = ROTOR_MODE_RAND;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_OPENCOVER)) {
    /* 每按一次开、关弹舱盖 */
    this->launcher_.cover_open = !this->launcher_.cover_open;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_AUTOAIM)) {
    if (this->ctrl_source_ == CMD_SOURCE_HOST) {
      /* 停止ai的自瞄模式，停用host控制 */
      this->ctrl_source_ = CMD_SOURCE_RC;
    } else {
      /* ai切换至自瞄模式，启用host控制 */
      this->ctrl_source_ = CMD_SOURCE_HOST;
    }
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_REVTRIG)) {
    /* 按下拨弹反转 */
    this->launcher_.reverse_trig = true;
  }
  if (this->BehaviorOccurred(CMD_BEHAVIOR_FOLLOWGIMBAL35)) {
    this->chassis_.mode = CHASSIS_MODE_FOLLOW_GIMBAL;
  }
  /* 保存当前按下的键位状态 */
  this->key_last_ = this->rc_.key;
  memcpy(&(this->mouse_last_), &(this->rc_.mouse), sizeof(this->mouse_last_));

  return true;
}

bool CMD::PraseJoystick() {
  switch (this->rc_.sw_l) {
      /* 左拨杆相应行为选择和解析 */
    case CMD_SW_UP:
      this->chassis_.mode = CHASSIS_MODE_BREAK;
      break;

    case CMD_SW_MID:
      this->chassis_.mode = CHASSIS_MODE_FOLLOW_GIMBAL;
      break;

    case CMD_SW_DOWN:
#if LEVER_R_DEFAULT
      this->chassis_.mode = this->param_.default_mode.chassis;
#elif LEVER_L_ROTATE
      this->chassis_.mode = CHASSIS_MODE_ROTOR;
#endif
      break;

    case CMD_SW_ERR:
      this->chassis_.mode = CHASSIS_MODE_RELAX;
      break;
  }
  switch (this->rc_.sw_r) {
      /* 右拨杆相应行为选择和解析 */
    case CMD_SW_UP:
      this->gimbal_.mode = GIMBAL_MODE_ABSOLUTE;
      this->launcher_.mode = LAUNCHER_MODE_SAFE;
      this->launcher_.fire_mode = FIRE_MODE_SINGLE;
      this->launcher_.fire = false;
      break;

    case CMD_SW_MID:
      this->gimbal_.mode = GIMBAL_MODE_ABSOLUTE;
      this->launcher_.mode = LAUNCHER_MODE_LOADED;
      this->launcher_.fire_mode = FIRE_MODE_SINGLE;
      this->launcher_.fire = false;
      break;

    case CMD_SW_DOWN:
      this->gimbal_.mode = this->param_.default_mode.gimbal;
      this->launcher_.mode = this->param_.default_mode.launcher;
#if LEVER_R_DEFAULT

#elif LEVER_R_FIRE_SINLE
      this->launcher_.fire = true;
      this->launcher_.fire_mode = FIRE_MODE_SINGLE;
#elif LEVER_R_FIRE_BURST
      this->launcher_.fire = true;
      this->launcher_.fire_mode = FIRE_MODE_BURST;
#elif LEVER_R_FIRE_CONT
      this->launcher_.fire = true;
      this->launcher_.fire_mode = FIRE_MODE_CONT;
#endif
      break;

    case CMD_SW_ERR:
      this->gimbal_.mode = GIMBAL_MODE_RELAX;
      this->launcher_.mode = LAUNCHER_MODE_RELAX;
  }
  /* 将操纵杆的对应值转换为底盘的控制向量和云台变化的欧拉角 */
  this->chassis_.ctrl_vec.vx = this->rc_.ch.l.x;
  this->chassis_.ctrl_vec.vy = this->rc_.ch.l.y;
  this->gimbal_.delta_eulr.yaw =
      this->rc_.ch.r.x * this->dt_ * this->param_.sens_stick;
  this->gimbal_.delta_eulr.pit =
      this->rc_.ch.r.y * this->dt_ * this->param_.sens_stick;

  return true;
}

void CMD::PackUI() {
  this->ui_.ctrl_method = this->ctrl_method_;
  this->ui_.ctrl_source = this->ctrl_source_;
}

void CMD::StopCtrl() {
  this->chassis_.mode = CHASSIS_MODE_RELAX;
  this->gimbal_.mode = GIMBAL_MODE_RELAX;
  this->launcher_.mode = LAUNCHER_MODE_RELAX;
}

CMD::Key CMD::BehaviorToKey(CMD::Behavior behavior) {
  return this->param_.key_map[behavior].key;
}

CMD::Activation CMD::BehaviorToActivation(CMD::Behavior behavior) {
  return this->param_.key_map[behavior].active;
}

bool CMD::BehaviorOccurred(CMD::Behavior behavior) {
  CMD::Key key = BehaviorToKey(behavior);
  CMD::Activation active = BehaviorToActivation(behavior);

  bool now_key_pressed, last_key_pressed;
  /* 鼠标左右键需要单独判断 */
  switch (key) {
    case CMD_KEY_L_CLICK:
      now_key_pressed = this->rc_.mouse.click.l;
      last_key_pressed = this->mouse_last_.click.l;
      break;

    case CMD_KEY_R_CLICK:
      now_key_pressed = this->rc_.mouse.click.r;
      last_key_pressed = this->mouse_last_.click.r;
      break;

    default:
      now_key_pressed = this->rc_.key & (1u << key);
      last_key_pressed = this->key_last_ & (1u << key);
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

bool CMD::RCLost() {
  return (this->rc_.sw_l == CMD_SW_ERR) || (this->rc_.sw_r == CMD_SW_ERR);
}
