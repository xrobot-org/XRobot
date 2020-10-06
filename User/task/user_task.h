#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ----------------------------------------------------------------- */
#include <cmsis_os2.h>

#include "FreeRTOS.h"
#include "module\config.h"
#include "task.h"

/* Exported constants ------------------------------------------------------- */
/* 所有任务都要define一个“任务运行频率”和“初始化延时” */
#define TASK_FREQ_HZ_CTRL_CHASSIS (1000u)
#define TASK_FREQ_HZ_CTRL_GIMBAL (1000u)
#define TASK_FREQ_HZ_CTRL_SHOOT (1000u)
#define TASK_FREQ_HZ_INFO (4u)
#define TASK_FREQ_HZ_MONITOR (2u)
#define TASK_FREQ_HZ_REFEREE (2u)

#define TASK_INIT_DELAY_COMMAND (15u)
#define TASK_INIT_DELAY_CTRL_CHASSIS (100)
#define TASK_INIT_DELAY_CTRL_GIMBAL (200)
#define TASK_INIT_DELAY_CTRL_SHOOT (300)
#define TASK_INIT_DELAY_INFO (500u)
#define TASK_INIT_DELAY_MONITOR (10)
#define TASK_INIT_DELAY_POSESTI (0u)
#define TASK_INIT_DELAY_REFEREE (400u)

/* Exported defines --------------------------------------------------------- */
/* Exported macro ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
typedef struct {
  struct {
    osThreadId_t cli;
    osThreadId_t command;
    osThreadId_t ctrl_chassis;
    osThreadId_t ctrl_gimbal;
    osThreadId_t ctrl_shoot;
    osThreadId_t info;
    osThreadId_t monitor;
    osThreadId_t atti_esti;
    osThreadId_t referee;
  } thread;

  struct {
    struct {
      osMessageQueueId_t accl;     /* IMU读取 */
      osMessageQueueId_t gyro;     /* IMU读取 */
      osMessageQueueId_t eulr_imu; /* 姿态解算得到 */
    } gimbal;

    struct {
      osMessageQueueId_t chassis;
      osMessageQueueId_t gimbal;
      osMessageQueueId_t shoot;
    } cmd;
  } msgq;

  struct {
    osMutexId_t atti_ready;
  } mutex;
  
  struct {
    float battery;
    float cpu_temp;
  } status;

  Config_t robot_id;
  const Config_Robot_t *config_robot;
  const Config_Pilot_t *config_pilot;

#ifdef DEBUG
  struct {
    UBaseType_t cli;
    UBaseType_t command;
    UBaseType_t ctrl_chassis;
    UBaseType_t ctrl_gimbal;
    UBaseType_t ctrl_shoot;
    UBaseType_t info;
    UBaseType_t monitor;
    UBaseType_t atti_esti;
    UBaseType_t referee;
  } stack_water_mark;

  struct {
    float cli;
    float command;
    float ctrl_chassis;
    float ctrl_gimbal;
    float ctrl_shoot;
    float info;
    float monitor;
    float atti_esti;
    float referee;
  } freq;

  struct {
    float cli;
    float command;
    float ctrl_chassis;
    float ctrl_gimbal;
    float ctrl_shoot;
    float info;
    float monitor;
    float atti_esti;
    float referee;
  } last_up_time;
#endif

} Task_Runtime_t;

extern Task_Runtime_t task_runtime;

extern const osThreadAttr_t attr_init;

extern const osThreadAttr_t attr_cli;
extern const osThreadAttr_t attr_command;
extern const osThreadAttr_t attr_ctrl_chassis;
extern const osThreadAttr_t attr_ctrl_gimbal;
extern const osThreadAttr_t attr_ctrl_shoot;
extern const osThreadAttr_t attr_info;
extern const osThreadAttr_t attr_monitor;
extern const osThreadAttr_t attr_atti_esti;
extern const osThreadAttr_t attr_referee;

/* Exported functions prototypes -------------------------------------------- */
void Task_Init(void *argument);

void Task_CLI(void *argument);
void Task_Command(void *argument);
void Task_CtrlChassis(void *argument);
void Task_CtrlGimbal(void *argument);
void Task_CtrlShoot(void *argument);
void Task_Info(void *argument);
void Task_Monitor(void *argument);
void Task_AttiEsti(void *argument);
void Task_Referee(void *argument);

#ifdef __cplusplus
}
#endif
