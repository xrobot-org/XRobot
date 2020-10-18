/*

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

typedef enum {
  ROBOT_MODEL_INFANTRY = 0, /* 步兵机器人 */
  ROBOT_MODEL_HERO,         /* 步兵机器人 */
  ROBOT_MODEL_ENGINEER,     /* 工程机器人 */
  ROBOT_MODEL_DRONE,        /* 空中机器人 */
  ROBOT_MODEL_SENTRY,       /* 哨兵机器人 */
  ROBOT_MODEL_NUM,          /* 型号数量 */
} Config_RobotModel_t;      /* 对应赛事规则的机器人型号 */

typedef enum {
  ROBOT_PILOT_QS = 0,
  ROBOT_PILOT_NUM,    /* 操作数量 */
} Config_PilotName_t; /* 操作员名称 */

typedef struct {
  Chassis_Params_t chassis; /* 底盘 */
  Gimbal_Params_t gimbal;   /* 云台 */
  Shoot_Params_t shoot;     /* 射击 */
} Config_RobotParam_t;      /* 机器人参数，保存后不会变化 */

typedef struct {
  struct {
    CMD_Params_t cmd; /* 遥控命令 */
  } param;            /* 参数 */

  struct {
    void (*MapChassis)(void);
    void (*MapGimbal)(void);
    void (*MapShoot)(void);
  } key_map; /* 键位映射 */

} Config_Pilot_t; /* 操作员配置 */

typedef struct {
  Config_RobotModel_t model;
  Config_PilotName_t pilot;

  struct {
    IST8310_Cali_t ist8310;
    BMI088_Cali_t bmi088;
  } cali; /* 校准 */

  AHRS_Eulr_t mech_zero_eulr; /* 机械零点 */

} Config_t; /* 机器人配置，保存在Flash上的信息，根据机器人变化 */

void Config_Get(Config_t *cfg);
void Config_Set(Config_t *cfg);
const Config_RobotParam_t *Config_GetRobotParam(Config_RobotModel_t model);
const Config_Pilot_t *Config_GetPilotCfg(Config_PilotName_t pilot);
Config_RobotModel_t Config_GetModelByName(const char *name);
Config_PilotName_t Config_GetPilotByName(const char *name);
const char *Config_GetNameByModel(Config_RobotModel_t model);
const char *Config_GetNameByPilot(Config_PilotName_t pilot);

#ifdef __cplusplus
}
#endif
