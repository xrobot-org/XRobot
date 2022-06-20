/*
 * 配置相关
 * 读写在FALSH上的信息
 */

#pragma once

#include <stdint.h>
#include <string>

#include "comp_cmd.hpp"
#include "comp_game.hpp"
#include "dev_bmi088.hpp"
#include "dev_ist8310.hpp"
#include "dev_motor.hpp"
#include "mod_chassis.hpp"
#include "mod_gimbal.hpp"
#include "mod_launcher.hpp"

/* clang-format on */
/* 机器人参数，保存后不会变化 */
typedef struct {
  std::string name;

  chassis_params_t chassis;   /* 底盘 */
  gimbal_params_t gimbal;     /* 云台 */
  launcher_params_t launcher; /* 发射器 */

  motor_group_t motor[MOTOR_GROUP_ID_NUM];

  bmi088_param_t imu;

  cap_param_t cap;
  tof_param_t tof;

  cmd_mode_group_t default_mode;

} config_robot_param_t;

/* 操作员配置 */
typedef struct {
  std::string name;
  cmd_params_t param; /* 参数 */
  ui_screen_t screen; /* 屏幕分辨率参数 */
} config_pilot_cfg_t;

/* 机器人配置，保存在Flash上的信息，根据机器人变化 */
typedef struct {
  const config_robot_param_t *robot_param;
  const config_pilot_cfg_t *pilot_cfg;

  struct {
    ist8310_cali_t ist8310;
    bmi088_cali_t bmi088;
  } cali; /* 校准 */

  eulr_t gimbal_mech_zero; /* 机械零点 */
  float gimbal_limit;      /* 云台pitch轴软件限位最高点 */

} config_t;

/* 机器人参数和对应字符串的映射  */
typedef struct {
  const char *const name;
  const config_robot_param_t *param;
} config_robot_param_map_t;

/* 操作手配置和对应字符串的映射 */
typedef struct {
  const char *const name;
  const config_pilot_cfg_t *param;
} config_pilot_cfg_map_t;

/**
 * @brief 从Flash读取配置信息
 *
 * @param cfg 配置信息
 */
void config_get(config_t *cfg);

/**
 * @brief 将配置信息写入Flash
 *
 * @param cfg 配置信息
 */
void config_set(config_t *cfg);

/**
 * @brief 通过机器人参数名称获取机器人参数的指针
 *
 * @param robot_param_name 机器人参数名称
 * @return const config_robot_param_t* 机器人参数的指针
 */
const config_robot_param_t *config_get_robot_param(
    const char *robot_param_name);

/**
 * @brief 通过操作手配置名称获取操作手配置的指针
 *
 * @param pilot_cfg_name 操作手配置名称
 * @return const config_pilot_cfg_t* 操作手配置的指针
 */
const config_pilot_cfg_t *config_get_pilot_cfg(const char *pilot_cfg_name);
const config_pilot_cfg_map_t *config_get_pilot_name_map(void);
const config_robot_param_map_t *config_get_robot_name_map(void);
