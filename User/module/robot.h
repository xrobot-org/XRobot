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
} Robot_Model_t;            /* 通过型号来获取对应的Robot_Config_t */

typedef enum {
  ROBOT_PILOT_QS = 0,
  ROBOT_PILOT_NUM, /* 操作数量 */
} Robot_Pilot_t;   /* 通过操作员来获取对应的Robot_PilotConfig_t */

typedef struct {
  Robot_Model_t model; /* 型号 */

  struct {
    Chassis_Params_t chassis; /* 底盘 */
    Gimbal_Params_t gimbal;   /* 云台 */
    Shoot_Params_t shoot;     /* 射击 */
  } param;                    /* 参数 */

} Robot_Config_t; /* 机器人配置 */

typedef struct {
  struct {
    CMD_Params_t cmd; /* 遥控命令 */
  } param;            /* 参数 */

  struct {
    void (*MapChassis)(void);
    void (*MapGimbal)(void);
    void (*MapShoot)(void);
  } key_map; /* 键位映射 */

} Robot_PilotConfig_t; /* 操作员配置 */

typedef struct {
  Robot_Model_t model;
  Robot_Pilot_t pilot;

  struct {
    IST8310_Cali_t ist8310;
    BMI088_Cali_t bmi088;
  } cali; /* 校准 */

} Robot_ID_t; /* 保存在Flash上的信息 */

void Robot_GetRobotID(Robot_ID_t *id);
void Robot_SetRobotID(Robot_ID_t *id);
const Robot_Config_t *Robot_GetConfig(Robot_Model_t model);
const Robot_PilotConfig_t *Robot_GetPilotConfig(Robot_Pilot_t pilot);
Robot_Model_t Robot_GetModelByName(const char *name);
Robot_Pilot_t Robot_GetPilotByName(const char *name);
const char *Robot_GetNameByModel(Robot_Model_t model);
const char *Robot_GetNameByPilot(Robot_Pilot_t pilot);

#ifdef __cplusplus
}
#endif
