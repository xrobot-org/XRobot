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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "comp_ahrs.hpp"

#define CMD_REFEREE_MAX_NUM (3) /* 发送命令限定的最大数量 */

#define GAME_HEAT_INCREASE_42MM (100.0f) /* 每发射一颗42mm弹丸增加100热量 */
#define GAME_HEAT_INCREASE_17MM (10.0f) /* 每发射一颗17mm弹丸增加10热量 */

#define GAME_CHASSIS_MAX_POWER_WO_REF 40.0f /* 裁判系统离线时底盘最大功率 */

namespace Component {
class CMD {
 public:
  /* 底盘运行模式 */
  typedef enum {
    CHASSIS_MODE_RELAX, /* 放松模式，电机不输出。一般情况底盘初始化之后的模式 */
    CHASSIS_MODE_BREAK, /* 刹车模式，电机闭环控制保持静止。用于机器人停止状态 */
    CHASSIS_MODE_FOLLOW_GIMBAL, /* 通过闭环控制使车头方向跟随云台 */
    CHASSIS_MODE_FOLLOW_GIMBAL_35, /* 通过闭环控制使车头方向35度跟随云台 */
    CHASSIS_MODE_ROTOR, /* 小陀螺模式，通过闭环控制使底盘不停旋转 */
    CHASSIS_MODE_INDENPENDENT, /* 独立模式。底盘运行不受云台影响 */
    CHASSIS_MODE_OPEN, /* 开环模式。底盘运行不受PID控制，直接输出到电机 */
    CHASSIS_MODE_SCAN, /*哨兵未找到目标，底盘处于自由活动模式*/
  } ChassisMode;

  /* 小陀螺转动模式 */
  typedef enum {
    ROTOR_MODE_CW,   /* 顺时针转动 */
    ROTOR_MODE_CCW,  /* 逆时针转动 */
    ROTOR_MODE_RAND, /* 随机转动 */
  } ChassisRotorMode;

  /* 云台运行模式 */
  typedef enum {
    GIMBAL_MODE_RELAX, /* 放松模式，电机不输出。一般情况云台初始化之后的模式 */
    GIMBAL_MODE_ABSOLUTE, /* 绝对坐标系控制，控制在空间内的绝对姿态 */
    GIMBAL_MODE_RELATIVE, /* 相对坐标系控制，控制相对于底盘的姿态 */
    GIMBAL_MODE_SCAN, /* 主动遍历每个角度，以便上位机识别 */
  } GimbalMode;

  /* 发射器运行模式 */
  typedef enum {
    LAUNCHER_MODE_RELAX, /* 放松模式，电机不输出 */
    LAUNCHER_MODE_SAFE,  /* 保险模式，电机闭环控制保持静止 */
    LAUNCHER_MODE_LOADED, /* 上膛模式，摩擦轮开启。随时准备开火 */
  } LauncherMode;

  /* 开火模式 */
  typedef enum {
    FIRE_MODE_SINGLE, /* 单发开火模式  */
    FIRE_MODE_BURST,  /* N爆发开火模式 */
    FIRE_MODE_CONT,   /* 持续开火模式 */
    FIRE_MODE_NUM,
  } FireMode;

  /* 底盘控制命令 */
  typedef struct {
    ChassisMode mode;                     /* 底盘运行模式 */
    ChassisRotorMode mode_rotor;          /* 小陀螺转动模式 */
    Component::Type::MoveVector ctrl_vec; /* 底盘控制向量 */
  } ChassisCMD;

  /* 云台控制命令 */
  typedef struct {
    GimbalMode mode;                  /* 云台运行模式 */
    Component::Type::Eulr delta_eulr; /* 欧拉角变化角度 */
  } GimbalCMD;

  /* 发射器控制命令 */
  typedef struct {
    LauncherMode mode;  /* 发射器运行模式 */
    FireMode fire_mode; /* 开火模式 */
    bool fire;          /*开火*/
    bool cover_open;    /* 弹舱盖开关 */
    bool reverse_trig;  /* 拨弹电机状态 */
  } LauncherCMD;

  typedef struct {
    GimbalMode gimbal;
    ChassisMode chassis;
    LauncherMode launcher;
  } ModeGroup;

  /* 拨杆位置 */
  typedef enum {
    CMD_SW_ERR = 0,
    CMD_SW_UP = 1,
    CMD_SW_MID = 3,
    CMD_SW_DOWN = 2,
  } SwitchPos;

  /* 键盘按键值 */
  typedef enum {
    CMD_KEY_W = 0,
    CMD_KEY_S,
    CMD_KEY_A,
    CMD_KEY_D,
    CMD_KEY_SHIFT,
    CMD_KEY_CTRL,
    CMD_KEY_Q,
    CMD_KEY_E,
    CMD_KEY_R,
    CMD_KEY_F,
    CMD_KEY_G,
    CMD_KEY_Z,
    CMD_KEY_X,
    CMD_KEY_C,
    CMD_KEY_V,
    CMD_KEY_B,
    CMD_KEY_L_CLICK,
    CMD_KEY_R_CLICK,
    CMD_KEY_NUM,
  } Key;

  /* 所有的机器人行为 */
  typedef enum {
    CMD_BEHAVIOR_FORE = 0,       /* 向前 */
    CMD_BEHAVIOR_BACK,           /* 向后 */
    CMD_BEHAVIOR_LEFT,           /* 向左 */
    CMD_BEHAVIOR_RIGHT,          /* 向右 */
    CMD_BEHAVIOR_ACCELERATE,     /* 加速 */
    CMD_BEHAVIOR_DECELEBRATE,    /* 减速 */
    CMD_BEHAVIOR_FIRE,           /* 开火 */
    CMD_BEHAVIOR_FIRE_MODE,      /* 切换开火模式 */
    CMD_BEHAVIOR_AUTOAIM,        /* 自瞄模式 */
    CMD_BEHAVIOR_OPENCOVER,      /* 弹舱盖开关 */
    CMD_BEHAVIOR_ROTOR,          /* 小陀螺模式 */
    CMD_BEHAVIOR_REVTRIG,        /* 反转拨弹 */
    CMD_BEHAVIOR_FOLLOWGIMBAL35, /* 跟随云台呈35度 */
    CMD_BEHAVIOR_NUM,
  } Behavior;

  /* 行为触发方式 */
  typedef enum {
    CMD_ACTIVE_PRESSING,  /* 按下时触发 */
    CMD_ACTIVE_RELEASING, /* 抬起时触发 */
    CMD_ACTIVE_PRESSED,   /* 按住时触发 */
  } Activation;

  typedef struct {
    Activation active;
    Key key;
  } KeyMapItem;

  /* 位移灵敏度参数 */
  typedef struct {
    float sense_norm; /* 移动灵敏度 */
    float sense_fast; /* 加速灵敏度 */
    float sense_slow; /* 减速灵敏度 */
  } MoveParam;

  /* 命令参数 */
  typedef struct {
    float sens_mouse;                     /* 鼠标灵敏度 */
    float sens_stick;                     /* 遥控器摇杆灵敏度 */
    KeyMapItem key_map[CMD_BEHAVIOR_NUM]; /* 按键映射行为命令 */
    MoveParam move;                       /* 位移灵敏度参数 */
    ModeGroup default_mode;
  } Param;

  typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;

    /* 键位 */
    struct {
      bool l; /* 左键 */
      bool r; /* 右键 */
    } click;
  } Mouse; /* 鼠标值 */

  /* 控制指令来源 */
  typedef enum {
    CMD_SOURCE_RC,   /* 指令来源于遥控链路 */
    CMD_SOURCE_HOST, /* 指令来源于上位机 */
  } CtrlSource;

  /* 控制方式 */
  typedef enum {
    CMD_METHOD_JOYSTICK_SWITCH, /* 使用摇杆拨杆控制 */
    CMD_METHOD_MOUSE_KEYBOARD,  /* 使用键鼠控制 */
  } CtrlMethod;

  typedef struct {
    struct {
      Component::Type::Vector2
          l; /* 遥控器左侧摇杆横轴值(x)，上为正；纵轴值(y)，右为正 */
      Component::Type::Vector2
          r; /* 遥控器右侧摇杆横轴值(x)，上为正；纵轴值(y)，右为正 */
    } ch;

    float ch_res; /* 第五通道值 */

    SwitchPos sw_r; /* 右侧拨杆位置 */
    SwitchPos sw_l; /* 左侧拨杆位置 */

    Mouse mouse; /* 鼠标值 */

    uint16_t key; /* 按键值 */

    uint16_t res; /* 保留，未启用 */
  } RC;

  typedef struct {
    Component::Type::Eulr gimbal_delta; /* 欧拉角的变化量 */

    Component::Type::MoveVector chassis_move_vec; /* 底盘移动向量 */

    bool fire; /* 开火状态 */
  } Host;

  CMD(Param &param);

  bool HostCtrl();

  bool PraseRC();

  bool PraseHost();

  bool PraseMouseKey(Key key);

  bool PraseKeyboard();

  bool PraseJoystick();

  void PackUI();

  void StopCtrl();

  Key BehaviorToKey(Behavior behavior);

  Activation BehaviorToActivation(Behavior behavior);

  bool BehaviorOccurred(Behavior behavior);

  bool RCLost();

  /* UI所用行为状态 */
  typedef struct {
    CtrlMethod ctrl_method;
    CtrlSource ctrl_source;
  } UI;

  uint32_t now_;
  uint32_t last_online_time_;
  float dt_;

  Param &param_; /* 命令参数 */

  CtrlSource ctrl_source_; /* 指令来源 */
  CtrlMethod ctrl_method_; /* 控制方式 */

  uint16_t key_last_; /* 上次按键键值 */
  Mouse mouse_last_;  /* 鼠标值 */

  ChassisCMD chassis_;   /* 底盘控制命令 */
  GimbalCMD gimbal_;     /* 云台控制命令 */
  LauncherCMD launcher_; /* 发射器控制命令 */

  RC rc_;

  Host host_;

  UI ui_;

  System::Thread thread_;
};
}  // namespace Component
