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

/* Includes ----------------------------------------------------------------- */
#include "FreeRTOS.h"
#include "mod_config.h"
#include "task.h"

/* Exported constants ------------------------------------------------------- */

/* 所有任务都要define一个“任务运行频率”和“初始化延时” */
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

/* Exported defines --------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */

typedef struct {
  /* 各任务，也可以叫做线程 */
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

  struct {
    /* 云台相关数据 */
    struct {
      QueueHandle_t accl;     /* IMU读取 */
      QueueHandle_t gyro;     /* IMU读取 */
      QueueHandle_t eulr_imu; /* 姿态解算得到 */
    } gimbal;

    /* 控制指令 */
    struct {
      /* 控制指令来源 */
      struct {
        QueueHandle_t host;
        QueueHandle_t rc;
      } src;

      QueueHandle_t chassis;
      QueueHandle_t gimbal;
      QueueHandle_t launcher;
      QueueHandle_t ai;
    } cmd;

    /* can任务放入、读取，电机或电容的输入输出 */
    struct {
      struct {
        QueueHandle_t chassis;
        QueueHandle_t gimbal;
        QueueHandle_t launcher;
        QueueHandle_t cap;
      } output;

      struct {
        QueueHandle_t chassis;
        QueueHandle_t gimbal;
        QueueHandle_t launcher;
        QueueHandle_t cap;
        QueueHandle_t tof;
      } feedback;
    } can;

    struct {
      QueueHandle_t quat; /* 姿态解算得到 */
    } ai;

    /* 裁判系统发送的 */
    struct {
      QueueHandle_t cap;
      QueueHandle_t chassis;
      QueueHandle_t ai;
      QueueHandle_t launcher;
    } referee;

    QueueHandle_t cap_info;

    struct {
      QueueHandle_t chassis;
      QueueHandle_t gimbal;
      QueueHandle_t launcher;
      QueueHandle_t cap;
      QueueHandle_t cmd;
      QueueHandle_t ai;
    } ui;

  } msgq;

  /* 机器人状态 */
  struct {
    float battery;
    float vbat;
    float cpu_temp;
  } status;

  Config_t cfg; /* 机器人配置 */

#ifdef MCU_DEBUG_BUILD

  /* stack使用 */
  struct {
    UBaseType_t cli;
    UBaseType_t cmd;
    UBaseType_t ctrl_cap;
    UBaseType_t ctrl_chassis;
    UBaseType_t ctrl_gimbal;
    UBaseType_t ctrl_launcher;
    UBaseType_t info;
    UBaseType_t monitor;
    UBaseType_t can;
    UBaseType_t atti_esti;
    UBaseType_t referee;
    UBaseType_t ai;
    UBaseType_t rc;
  } stack_water_mark;
#endif
} Runtime_t;

extern Runtime_t runtime;

/* Exported functions prototypes -------------------------------------------- */
void Thread_Init(void);
