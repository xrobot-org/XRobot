#pragma once

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "task.h"

/* Exported constants --------------------------------------------------------*/
/* 所有任务都要define一个“任务运行频率”和“初始化延时”。 */
#define TASK_FREQ_HZ_CLI					(5u)
#define TASK_FREQ_HZ_COMMAND				(80u)
#define TASK_FREQ_HZ_CTRL_CHASSIS			(50u)
#define TASK_FREQ_HZ_CTRL_GIMBAL			(50u)
#define TASK_FREQ_HZ_CTRL_SHOOT				(50u)
#define TASK_FREQ_HZ_INFO					(2u)
#define TASK_FREQ_HZ_MONITOR				(2u)
#define TASK_FREQ_HZ_POSESTI				(200u)
#define TASK_FREQ_HZ_REFEREE				(2u)

#define TASK_INIT_DELAY_CLI					(5u)
#define TASK_INIT_DELAY_COMMAND				(15u)
#define TASK_INIT_DELAY_CTRL_CHASSIS		(100)
#define TASK_INIT_DELAY_CTRL_GIMBAL			(200)
#define TASK_INIT_DELAY_CTRL_SHOOT			(300)
#define TASK_INIT_DELAY_INFO				(500u)
#define TASK_INIT_DELAY_MONITOR				(10)
#define TASK_INIT_DELAY_POSESTI				(0u)
#define TASK_INIT_DELAY_REFEREE				(400u)

/* Exported defines ----------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	struct {
		osThreadId_t cli;
		osThreadId_t command;
		osThreadId_t ctrl_chassis;
		osThreadId_t ctrl_gimbal;
		osThreadId_t ctrl_shoot;
		osThreadId_t info;
		osThreadId_t monitor;
		osThreadId_t pos_esti;
		osThreadId_t referee;
	} thread;
	
	struct {
		/* Pos esti */
		osMessageQueueId_t gimb_eulr;
	} messageq;
	
} Task_Param_t;

/* Exported functions prototypes ---------------------------------------------*/
void Task_CLI(void *argument);
void Task_Command(void *argument);
void Task_CtrlChassis(void *argument);
void Task_CtrlGimbal(void *argument);
void Task_CtrlShoot(void *argument);
void Task_Info(void *argument);
void Task_Monitor(void *argument);
void Task_PosEsti(void *argument);
void Task_Referee(void *argument);

