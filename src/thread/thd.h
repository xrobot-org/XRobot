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
  THREAD_AI,
  THREAD_ATTI_ESTI,
  THREAD_CAN,
  THREAD_CLI,
  THREAD_CMD,
  THREAD_CTRL_CAP,
  THREAD_CTRL_CHASSIS,
  THREAD_CTRL_GIMBAL,
  THREAD_CTRL_LAUNCHER,
  THREAD_IMU,
  THREAD_INFO,
  THREAD_MONITOR,
  THREAD_MSG_DISTRIB,
  THREAD_RC,
  THREAD_REFEREE,
  THREAD_USB,
  THREAD_NUM,
} Thread_Name_t;

/* 所有线程都要define一个“线程运行频率”和“初始化延时” */
#define TASK_FREQ_ATTI_ESTI (500u)
#define TASK_FREQ_CTRL_CHASSIS (500u)
#define TASK_FREQ_CTRL_GIMBAL (500u)
#define TASK_FREQ_CTRL_LAUNCHER (500u)
#define TASK_FREQ_CTRL_CAP (100u)
#define TASK_FREQ_CTRL_COMMAND (500u)
#define TASK_FREQ_INFO (4u)
#define TASK_FREQ_MONITOR (2u)
#define TASK_FREQ_CAN (1000u)
#define TASK_FREQ_AI (500u)
#define TASK_FREQ_REFEREE (1000u)

typedef struct {
  TaskHandle_t thread[THREAD_NUM];

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

void Thread_Init(void);
