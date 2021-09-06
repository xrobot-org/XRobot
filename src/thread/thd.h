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

#include "FreeRTOS.h"
#include "mod_config.h"
#include "task.h"

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
  /* 各线程，也可以叫做线程 */
  struct {
    TaskHandle_t cli;
    TaskHandle_t cmd;
    TaskHandle_t ctrl_cap;
    TaskHandle_t ctrl_chassis;
    TaskHandle_t ctrl_gimbal;
    TaskHandle_t ctrl_launcher;
    TaskHandle_t info;
    TaskHandle_t monitor;
    TaskHandle_t can;
    TaskHandle_t atti_esti;
    TaskHandle_t referee;
    TaskHandle_t ai;
    TaskHandle_t rc;
  } thread;

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

/* Exported functions prototypes -------------------------------------------- */
void Thread_Init(void);
