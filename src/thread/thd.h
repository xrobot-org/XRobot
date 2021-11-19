/**
 * @file user_task.h
 * @author Qu Shen (503578404@qq.com)
 * @brief
 * @version 1.0.0
 * @date 2021-04-15
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#include "mod_config.h"

typedef enum {
  THD_AI,
  THD_ATTI_ESTI,
  THD_CAN,
  THD_CLI,
  THD_CMD,
  THD_CTRL_CAP,
  THD_CTRL_CHASSIS,
  THD_CTRL_GIMBAL,
  THD_CTRL_LAUNCHER,
  THD_IMU,
  THD_INFO,
  THD_MONITOR,
  THD_MOTOR,
  THD_MSG_DISTRIB,
  THD_RC,
  THD_REFEREE,
  THD_TOF,
  THD_USB,
  THD_NUM,
} Thd_Name_t;

typedef struct {
  TaskHandle_t thd[THD_NUM];

  /* 机器人状态 */
  struct {
    float battery;
    float vbat;
    float cpu_temp;
  } status;

  Config_t cfg; /* 机器人配置 */

#ifdef MCU_DEBUG_BUILD

#endif
} Runtime_t;

void Thd_Init(void);
