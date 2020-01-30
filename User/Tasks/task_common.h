#pragma once

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os.h"

/* Exported constants --------------------------------------------------------*/
/* Exported defines ----------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId cli;
	osThreadId comm;
	osThreadId ctrl_chassis;
	osThreadId ctrl_gimbal;
	osThreadId ctrl_shoot;
	osThreadId debug;
	osThreadId info;
	osThreadId monitor;
	osThreadId pos_esti;
	osThreadId referee;
} Task_List_t;


/* 所有任务都要define一个“任务运行频率”和“初始化延时”。 */
#define TASK_CLI_FREQ_HZ (5u)
#define TASK_CLI_INIT_DELAY (1000u)

#define TASK_CAN_FREQ_HZ (50u)
#define TASK_CAN_INIT_DELAY (500u)

#define TASK_CTRL_CHASSIS_FREQ_HZ (50u)
#define TASK_CTRL_CHASSIS_INIT_DELAY (500u)

#define TASK_DEBUG_FREQ_HZ (50u)
#define TASK_DEBUG_INIT_DELAY (500u)


#define TASK_INFO_FREQ_HZ (2u)
#define TASK_INFO_INIT_DELAY (1000u)

#define TASK_POSESTI_FREQ_HZ (200)
#define TASK_POSESTI_INIT_DELAY (10)

/* Task_CLI */
/* Task_Comm */
/* Task_CtrlChassis */
/* Task_CtrlGimbal */
/* Task_CtrlShoot */
/* Task_Debug */
/* Task_Detect */
/* Task_Info */
/* Task_Init */
/* Task_Monitor */
/* Task_Output */
/* Task_PosEsti */
/* Task_Referee */

void Task_CAN(const void *argument);
void Task_CLI(const void *argument);
void Task_Comm(const void *argument);
void Task_CtrlChassis(const void *argument);
void Task_CtrlGimbal(const void *argument);
void Task_CtrlShoot(const void *argument);
void Task_Debug(const void *argument);
void Task_Detect(const void *argument);
void Task_Info(const void *argument);
void Task_Init(const void *argument);
void Task_Monitor(const void *argument);
void Task_Output(const void *argument);
void Task_PosEsti(const void *argument);
void Task_Referee(const void *argument);

/* Exported functions prototypes ---------------------------------------------*/
