/*
 * 配置相关
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "component\cmd.h"
#include "device\bmi088.h"
#include "device\ist8310.h"
#include "module\chassis.h"
#include "module\gimbal.h"
#include "module\shoot.h"

/* 对应赛事规则的机器人型号 */
typedef enum {
  ROBOT_MODEL_INFANTRY = 0, /* 步兵机器人 */
  ROBOT_MODEL_HERO,         /* 英雄机器人 */
  ROBOT_MODEL_ENGINEER,     /* 工程机器人 */
  ROBOT_MODEL_DRONE,        /* 空中机器人 */
  ROBOT_MODEL_SENTRY,       /* 哨兵机器人 */
  ROBOT_MODEL_NUM,          /* 型号数量 */
} Config_RobotModel_t;

/* 操作员名称 */
typedef enum {
  ROBOT_PILOT_QS = 0,
  ROBOT_PILOT_NUM, /* 操作数量 */
} Config_PilotName_t;

/* 机器人参数，保存后不会变化 */
typedef struct {
  Chassis_Params_t chassis; /* 底盘 */
  Gimbal_Params_t gimbal;   /* 云台 */
  Shoot_Params_t shoot;     /* 射击 */
} Config_RobotParam_t;

/* 操作员配置 */
typedef struct {
  struct {
    CMD_Params_t cmd; /* 遥控命令 */
  } param;            /* 参数 */

  struct {
    void (*MapChassis)(void);
    void (*MapGimbal)(void);
    void (*MapShoot)(void);
  } key_map; /* 键位映射 */

} Config_Pilot_t;

/* 机器人配置，保存在Flash上的信息，根据机器人变化 */
typedef struct {
  Config_RobotModel_t model;
  Config_PilotName_t pilot;

  struct {
    IST8310_Cali_t ist8310;
    BMI088_Cali_t bmi088;
  } cali; /* 校准 */

  AHRS_Eulr_t mech_zero; /* 机械零点 */

} Config_t;

/*!
 * \brief 从Flash读取配置信息
 *
 * \param cfg 配置信息
 */
void Config_Get(Config_t *cfg);

/*!
 * \brief 将配置信息写入Flash
 *
 * \param cfg 配置信息
 */
void Config_Set(Config_t *cfg);

/*!
 * \brief 获取机器人参数
 *
 * \param model 机器人型号
 *
 * \return 机器人参数
 */
const Config_RobotParam_t *Config_GetRobotParam(Config_RobotModel_t model);

/*!
 * \brief 获取操作手配置
 *
 * \param pilot 操作手
 *
 * \return 操作手配置
 */
const Config_Pilot_t *Config_GetPilotCfg(Config_PilotName_t pilot);

/*!
 * \brief 通过字符串获得机器人型号
 *
 * \param name 名字字符串
 *
 * \return 机器人模型
 */
Config_RobotModel_t Config_GetModelByName(const char *name);

/*!
 * \brief 通过字符串获得操作手
 *
 * \param name 名字字符串
 *
 * \return 操作手
 */
Config_PilotName_t Config_GetPilotByName(const char *name);

/*!
 * \brief 获得机器人型号对应名字字符串
 *
 * \param model 机器人型号
 *
 * \return 名字字符串
 */
const char *Config_GetNameByModel(Config_RobotModel_t model);

/*!
 * \brief 获得操作手对应名字字符串
 *
 * \param pilot 操作手
 *
 * \return 名字字符串
 */
const char *Config_GetNameByPilot(Config_PilotName_t pilot);

#ifdef __cplusplus
}
#endif
