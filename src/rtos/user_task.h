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
#include <cmsis_os2.h>

#include "FreeRTOS.h"
#include "module/config.h"
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
    osThreadId_t cli;
    osThreadId_t cmd;
    osThreadId_t ctrl_cap;
    osThreadId_t ctrl_chassis;
    osThreadId_t ctrl_gimbal;
    osThreadId_t ctrl_launcher;
    osThreadId_t info;
    osThreadId_t monitor;
    osThreadId_t can;
    osThreadId_t atti_esti;
    osThreadId_t referee;
    osThreadId_t ai;
    osThreadId_t rc;
  } thread;

  struct {
    /* 云台相关数据 */
    struct {
      osMessageQueueId_t accl;     /* IMU读取 */
      osMessageQueueId_t gyro;     /* IMU读取 */
      osMessageQueueId_t eulr_imu; /* 姿态解算得到 */
    } gimbal;

    /* 控制指令 */
    struct {
      /* 控制指令来源 */
      struct {
        osMessageQueueId_t host;
        osMessageQueueId_t rc;
      } src;

      osMessageQueueId_t chassis;
      osMessageQueueId_t gimbal;
      osMessageQueueId_t launcher;
      osMessageQueueId_t ai;
    } cmd;

    /* can任务放入、读取，电机或电容的输入输出 */
    struct {
      struct {
        osMessageQueueId_t chassis;
        osMessageQueueId_t gimbal;
        osMessageQueueId_t launcher;
        osMessageQueueId_t cap;
      } output;

      struct {
        osMessageQueueId_t chassis;
        osMessageQueueId_t gimbal;
        osMessageQueueId_t launcher;
        osMessageQueueId_t cap;
        osMessageQueueId_t tof;
      } feedback;
    } can;

    struct {
      osMessageQueueId_t quat; /* 姿态解算得到 */
    } ai;

    /* 裁判系统发送的 */
    struct {
      osMessageQueueId_t cap;
      osMessageQueueId_t chassis;
      osMessageQueueId_t ai;
      osMessageQueueId_t launcher;
    } referee;

    osMessageQueueId_t cap_info;

    struct {
      osMessageQueueId_t chassis;
      osMessageQueueId_t gimbal;
      osMessageQueueId_t launcher;
      osMessageQueueId_t cap;
      osMessageQueueId_t cmd;
      osMessageQueueId_t ai;
    } ui;

  } msgq;

  /* 机器人状态 */
  struct {
    float battery;
    float vbat;
    float cpu_temp;
  } status;

  Config_t cfg; /* 机器人配置 */

#ifdef DEBUG

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

extern const osThreadAttr_t attr_init;

extern const osThreadAttr_t attr_cli;
extern const osThreadAttr_t attr_cmd;
extern const osThreadAttr_t attr_ctrl_cap;
extern const osThreadAttr_t attr_ctrl_chassis;
extern const osThreadAttr_t attr_ctrl_gimbal;
extern const osThreadAttr_t attr_ctrl_launcher;
extern const osThreadAttr_t attr_info;
extern const osThreadAttr_t attr_monitor;
extern const osThreadAttr_t attr_can;
extern const osThreadAttr_t attr_atti_esti;
extern const osThreadAttr_t attr_referee;
extern const osThreadAttr_t attr_ai;
extern const osThreadAttr_t attr_rc;

/* Exported functions prototypes -------------------------------------------- */
void Task_Init(void *argument);

void Task_Ai(void *argument);
void Task_AttiEsti(void *argument);
void Task_Can(void *argument);
void Task_CLI(void *argument);
void Task_Cmd(void *argument);
void Task_CtrlCap(void *argument);
void Task_CtrlChassis(void *argument);
void Task_CtrlGimbal(void *argument);
void Task_CtrlLauncher(void *argument);
void Task_Info(void *argument);
void Task_Monitor(void *argument);
void Task_RC(void *argument);
void Task_Referee(void *argument);
