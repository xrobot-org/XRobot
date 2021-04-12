/*
  游戏规则相关
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define GAME_HEAT_INCREASE_42MM (100.f) /* 每发射一颗42mm弹丸增加100热量 */
#define GAME_HEAT_INCREASE_17MM (10.f) /* 每发射一颗17mm弹丸增加10热量 */

/* 机器人型号 */
typedef enum {
  ROBOT_MODEL_INFANTRY = 0, /* 步兵机器人 */
  ROBOT_MODEL_HERO,         /* 步兵机器人 */
  ROBOT_MODEL_ENGINEER,     /* 工程机器人 */
  ROBOT_MODEL_DRONE,        /* 空中机器人 */
  ROBOT_MODEL_SENTRY,       /* 哨兵机器人 */
  ROBOT_MODEL_NUM,          /* 型号数量 */
} Game_RobotModel_t;

#ifdef __cplusplus
}
#endif
