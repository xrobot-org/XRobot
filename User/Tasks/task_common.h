#pragma once

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"
#include "FreeRTOS.h"

/* Exported constants --------------------------------------------------------*/
#define TASK_SIGNAL_CLI_READY					(1u<<0)
#define TASK_SIGNAL_COMMAND_READY					(1u<<1)
#define TASK_SIGNAL_CTRL_CHASSIS_READY			(1u<<2)
#define TASK_SIGNAL_CTRL_GIMBAL_READY			(1u<<3)
#define TASK_SIGNAL_CTRL_SHOOT_READY			(1u<<4)
#define TASK_SIGNAL_DEBUG_READY					(1u<<5)
#define TASK_SIGNAL_INFO_READY					(1u<<6)
#define TASK_SIGNAL_MONITOR_READY				(1u<<7)
#define TASK_SIGNAL_POSESTI_READY				(1u<<8)
#define TASK_SIGNAL_REFEREE_READY				(1u<<9)

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
		osThreadId cli;
		osThreadId command;
		osThreadId ctrl_chassis;
		osThreadId ctrl_gimbal;
		osThreadId ctrl_shoot;
		osThreadId info;
		osThreadId monitor;
		osThreadId pos_esti;
		osThreadId referee;
	} thread;
	
	struct {
		/* Pos esti */
		osMessageQId gimb_eulr;
	} message;
	
} Task_Param_t;

/* Exported functions prototypes ---------------------------------------------*/
void Task_CLI(void const *argument);
void Task_Command(void const *argument);
void Task_CtrlChassis(void const *argument);
void Task_CtrlGimbal(void const *argument);
void Task_CtrlShoot(void const *argument);
void Task_Info(void const *argument);
void Task_Monitor(void const *argument);
void Task_PosEsti(void const *argument);
void Task_Referee(void const *argument);

