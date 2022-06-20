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
#include "comp_game.hpp"

#define CMD_REFEREE_MAX_NUM (3) /* 发送命令限定的最大数量 */

/* 底盘控制命令 */
typedef struct {
  chassis_mode_t mode;             /* 底盘运行模式 */
  chassis_rotor_mode_t mode_rotor; /* 小陀螺转动模式 */
  move_vector_t ctrl_vec;          /* 底盘控制向量 */
} cmd_chassis_t;

/* 云台控制命令 */
typedef struct {
  gimbal_mode_t mode; /* 云台运行模式 */
  eulr_t delta_eulr;  /* 欧拉角变化角度 */
} cmd_gimbal_t;

/* 发射器控制命令 */
typedef struct {
  launcher_mode_t mode;  /* 发射器运行模式 */
  fire_mode_t fire_mode; /* 开火模式 */
  bool fire;             /*开火*/
  bool cover_open;       /* 弹舱盖开关 */
  bool reverse_trig;     /* 拨弹电机状态 */
} cmd_launcher_t;

typedef struct {
  gimbal_mode_t gimbal;
  chassis_mode_t chassis;
  launcher_mode_t launcher;
} cmd_mode_group_t;

/* 拨杆位置 */
typedef enum {
  CMD_SW_ERR = 0,
  CMD_SW_UP = 1,
  CMD_SW_MID = 3,
  CMD_SW_DOWN = 2,
} cmd_switch_pos_t;

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
} cmd_key_t;

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
} cmd_behavior_t;

/* 行为触发方式 */
typedef enum {
  CMD_ACTIVE_PRESSING,  /* 按下时触发 */
  CMD_ACTIVE_RELEASING, /* 抬起时触发 */
  CMD_ACTIVE_PRESSED,   /* 按住时触发 */
} cmd_activation_t;

typedef struct {
  cmd_activation_t active;
  cmd_key_t key;
} cmd_key_map_item_t;

/* 位移灵敏度参数 */
typedef struct {
  float sense_norm; /* 移动灵敏度 */
  float sense_fast; /* 加速灵敏度 */
  float sense_slow; /* 减速灵敏度 */
} cmd_move_params_t;

/* 命令参数 */
typedef struct {
  float sens_mouse;                             /* 鼠标灵敏度 */
  float sens_stick;                             /* 遥控器摇杆灵敏度 */
  cmd_key_map_item_t key_map[CMD_BEHAVIOR_NUM]; /* 按键映射行为命令 */
  cmd_move_params_t move;                       /* 位移灵敏度参数 */
} cmd_params_t;

typedef struct {
  int16_t x;
  int16_t y;
  int16_t z;

  /* 键位 */
  struct {
    bool l; /* 左键 */
    bool r; /* 右键 */
  } click;
} cmd_mouse_t; /* 鼠标值 */

/* 控制指令来源 */
typedef enum {
  CMD_SOURCE_RC,   /* 指令来源于遥控链路 */
  CMD_SOURCE_HOST, /* 指令来源于上位机 */
} cmd_ctrl_source_t;

/* 控制方式 */
typedef enum {
  CMD_METHOD_JOYSTICK_SWITCH, /* 使用摇杆拨杆控制 */
  CMD_METHOD_MOUSE_KEYBOARD,  /* 使用键鼠控制 */
} cmd_ctrl_method_t;

typedef struct {
  cmd_ctrl_source_t ctrl_source; /* 指令来源 */
  cmd_ctrl_method_t ctrl_method; /* 控制方式 */

  uint16_t key_last;      /* 上次按键键值 */
  cmd_mouse_t mouse_last; /* 鼠标值 */

  const cmd_params_t *param; /* 命令参数 */
  const cmd_mode_group_t *def_mode;

  cmd_chassis_t chassis;   /* 底盘控制命令 */
  cmd_gimbal_t gimbal;     /* 云台控制命令 */
  cmd_launcher_t launcher; /* 发射器控制命令 */
} cmd_t;

typedef struct {
  struct {
    vector2_t l; /* 遥控器左侧摇杆横轴值(x)，上为正；纵轴值(y)，右为正 */
    vector2_t r; /* 遥控器右侧摇杆横轴值(x)，上为正；纵轴值(y)，右为正 */
  } ch;

  float ch_res; /* 第五通道值 */

  cmd_switch_pos_t sw_r; /* 右侧拨杆位置 */
  cmd_switch_pos_t sw_l; /* 左侧拨杆位置 */

  cmd_mouse_t mouse; /* 鼠标值 */

  uint16_t key; /* 按键值 */

  uint16_t res; /* 保留，未启用 */
} cmd_rc_t;

typedef struct {
  eulr_t gimbal_delta; /* 欧拉角的变化量 */

  struct {
    float vx;         /* x轴移动速度 */
    float vy;         /* y轴移动速度 */
    float wz;         /* z轴转动速度 */
  } chassis_move_vec; /* 底盘移动向量 */

  bool fire; /* 开火状态 */
} cmd_host_t;

/* UI所用行为状态 */
typedef struct {
  cmd_ctrl_method_t ctrl_method;
  cmd_ctrl_source_t ctrl_source;
} cmd_ui_t;

/**
 * @brief 解析行为命令
 *
 * @param rc 遥控器数据
 * @param cmd 控制指令数据
 */
int8_t cmd_init(cmd_t *cmd, const cmd_params_t *param,
                const cmd_mode_group_t *def_mode);

/**
 * @brief 检查是否启用上位机控制指令覆盖
 *
 * @param rc 遥控链路数据
 * @param cmd 控制指令数据
 * @return true 启用
 * @return false 不启用
 */
bool cmd_check_host_overwrite(const cmd_rc_t *rc, cmd_t *cmd);

/**
 * @brief 解析命令
 *
 * @param rc 遥控器数据
 * @param cmd 控制指令数据
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t cmd_parse_rc(const cmd_rc_t *rc, cmd_t *cmd, float dt_sec);

/**
 * @brief 解析上位机命令
 *
 * @param host host数据
 * @param cmd 控制指令数据
 * @param dt_sec 两次解析的间隔
 * @return int8_t 0对应没有错误
 */
int8_t cmd_parse_host(const cmd_host_t *host, cmd_t *cmd, float dt_sec);

/**
 * @brief 导出控制指令UI数据
 *
 * @param cmd_ui 控制指令UI数据
 * @param cmd 控制指令数据
 */
void cmd_pack_ui(cmd_ui_t *cmd_ui, const cmd_t *cmd);
