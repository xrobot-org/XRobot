/**
 * @file game.h
 * @author Qu Shen
 * @brief 游戏规则相关
 * @version 1.0.0
 * @date 2021-04-23
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#define GAME_HEAT_INCREASE_42MM (100.0f) /* 每发射一颗42mm弹丸增加100热量 */
#define GAME_HEAT_INCREASE_17MM (10.0f) /* 每发射一颗17mm弹丸增加10热量 */

#define GAME_CHASSIS_MAX_POWER_WO_REF 40.0f /* 裁判系统离线时底盘最大功率 */

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
} chassis_mode_t;

/* 小陀螺转动模式 */
typedef enum {
  ROTOR_MODE_CW,   /* 顺时针转动 */
  ROTOR_MODE_CCW,  /* 逆时针转动 */
  ROTOR_MODE_RAND, /* 随机转动 */
} chassis_rotor_mode_t;

/* 云台运行模式 */
typedef enum {
  GIMBAL_MODE_RELAX, /* 放松模式，电机不输出。一般情况云台初始化之后的模式 */
  GIMBAL_MODE_ABSOLUTE, /* 绝对坐标系控制，控制在空间内的绝对姿态 */
  GIMBAL_MODE_RELATIVE, /* 相对坐标系控制，控制相对于底盘的姿态 */
  GIMBAL_MODE_SCAN,     /* 主动遍历每个角度，以便上位机识别 */
} gimbal_mode_t;

/* 发射器运行模式 */
typedef enum {
  LAUNCHER_MODE_RELAX,  /* 放松模式，电机不输出 */
  LAUNCHER_MODE_SAFE,   /* 保险模式，电机闭环控制保持静止 */
  LAUNCHER_MODE_LOADED, /* 上膛模式，摩擦轮开启。随时准备开火 */
} launcher_mode_t;

/* 开火模式 */
typedef enum {
  FIRE_MODE_SINGLE, /* 单发开火模式  */
  FIRE_MODE_BURST,  /* N爆发开火模式 */
  FIRE_MODE_CONT,   /* 持续开火模式 */
  FIRE_MODE_NUM,
} fire_mode_t;

/* 飞镖发射器运行模式 */
typedef enum {
  DART_LAUNCHER_MODE_RELAX, /* 放松模式，电机不输出 */
  DART_LAUNCHER_MODE_SAFE,  /* 保险模式，电机闭环控制保持静止 */
  DART_LAUNCHER_MODE_LOADED, /* 上膛模式，摩擦轮开启。随时准备开火 */
} dart_launcher_mode_t;
