/*
 * 配置相关
 * 读写在FALSH上的信息
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "component/cmd.h"
#include "component/game.h"
#include "device/bmi088.h"
#include "device/can.h"
#include "device/ist8310.h"
#include "module/chassis.h"
#include "module/gimbal.h"
#include "module/launcher.h"

/* 机器人参数，保存后不会变化 */
typedef struct {
  Game_RobotModel_t model;    /* 型号 */
  Chassis_Params_t chassis;   /* 底盘 */
  Gimbal_Params_t gimbal;     /* 云台 */
  Launcher_Params_t launcher; /* 发射器 */
  CAN_Params_t can;           /* 电机CAN配置 */
} Config_RobotParam_t;

/* 操作员配置 */
typedef struct {
  CMD_Params_t param; /* 参数 */
} Config_PilotCfg_t;

/* 机器人配置，保存在Flash上的信息，根据机器人变化 */
typedef struct {
  char robot_param_name[20];
  char pilot_cfg_name[20];

  const Config_RobotParam_t *robot_param;
  const Config_PilotCfg_t *pilot_cfg;

  struct {
    IST8310_Cali_t ist8310;
    BMI088_Cali_t bmi088;
  } cali; /* 校准 */

  AHRS_Eulr_t mech_zero; /* 机械零点 */
  float gimbal_limit;    /* 云台pitch轴软件限位最高点 */

} Config_t;

/* 机器人参数和对应字符串的映射  */
typedef struct {
  const char *const name;
  const Config_RobotParam_t *param;
} Config_RobotParamMap_t;

/* 操作手配置和对应字符串的映射 */
typedef struct {
  const char *const name;
  const Config_PilotCfg_t *param;
} Config_PilotCfgMap_t;

/**
 * \brief 从Flash读取配置信息
 *
 * \param cfg 配置信息
 */
void Config_Get(Config_t *cfg);

/**
 * \brief 将配置信息写入Flash
 *
 * \param cfg 配置信息
 */
void Config_Set(Config_t *cfg);

/**
 * @brief 通过机器人参数名称获取机器人参数的指针
 *
 * @param robot_param_name 机器人参数名称
 * @return const Config_RobotParam_t* 机器人参数的指针
 */
const Config_RobotParam_t *Config_GetRobotParam(const char *robot_param_name);

/**
 * @brief 通过操作手配置名称获取操作手配置的指针
 *
 * @param pilot_cfg_name 操作手配置名称
 * @return const Config_PilotCfg_t* 操作手配置的指针
 */
const Config_PilotCfg_t *Config_GetPilotCfg(const char *pilot_cfg_name);
const Config_PilotCfgMap_t *Config_GetPilotNameMap(void);
const Config_RobotParamMap_t *Config_GetRobotNameMap(void);
#ifdef __cplusplus
}
#endif
