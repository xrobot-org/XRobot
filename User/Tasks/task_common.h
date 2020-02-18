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
#define TASK_CLI_FREQ_HZ					(5u)
#define TASK_CLI_INIT_DELAY					(1000u)

#define TASK_COMMAND_FREQ_HZ					(80u)
#define TASK_COMMAND_INIT_DELAY				(20u)

#define TASK_CTRL_CHASSIS_FREQ_HZ			(50u)
#define TASK_CTRL_CHASSIS_INIT_DELAY		(500u)

#define TASK_CTRL_GIMBAL_FREQ_HZ			(50u)
#define TASK_CTRL_GIMBAL_INIT_DELAY			(500u)

#define TASK_CTRL_SHOOT_FREQ_HZ				(50u)
#define TASK_CTRL_SHOOT_INIT_DELAY			(500u)

#define TASK_DEBUG_FREQ_HZ					(50u)
#define TASK_DEBUG_INIT_DELAY				(500u)

#define TASK_INFO_FREQ_HZ					(2u)
#define TASK_INFO_INIT_DELAY				(100u)

#define TASK_MONITOR_FREQ_HZ				(2u)
#define TASK_MONITOR_INIT_DELAY				(1000u)

#define TASK_POSESTI_FREQ_HZ				(200u)
#define TASK_POSESTI_INIT_DELAY				(10u)

#define TASK_REFEREE_FREQ_HZ				(2u)
#define TASK_REFEREE_INIT_DELAY				(1000u)

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
		osPoolId ahrs;
		osPoolId chassis_ctrl_v;
		osPoolId gimb_eulr;
		osPoolId ctrl_eulr;
		osPoolId imu;
	} pool;
	
	struct {
		osMessageQId ahrs;
		osMessageQId chassis_ctrl_v;
		osMessageQId gimb_eulr;
		osMessageQId ctrl_eulr;
		osMessageQId imu;
	} message;
	
	
} Task_Param_t;

/* Exported functions prototypes ---------------------------------------------*/
int Task_InitParam(Task_Param_t *task_param);

void Task_CAN(void const *argument);
void Task_CLI(void const *argument);
void Task_Command(void const *argument);
void Task_CtrlChassis(void const *argument);
void Task_CtrlGimbal(void const *argument);
void Task_CtrlShoot(void const *argument);
void Task_Debug(void const *argument);
void Task_Detect(void const *argument);
void Task_Info(void const *argument);
void Task_Init(void const *argument);
void Task_Monitor(void const *argument);
void Task_Output(void const *argument);
void Task_PosEsti(void const *argument);
void Task_Referee(void const *argument);

