#pragma once

/* Includes ------------------------------------------------------------------*/
#include "cmsis_os2.h"

/* Exported constants --------------------------------------------------------*/
/* Exported defines ----------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
typedef struct {
	osThreadId_t cli;
	osThreadId_t comm;
	osThreadId_t ctrl_chassis;
	osThreadId_t ctrl_gimbal;
	osThreadId_t ctrl_shoot;
	osThreadId_t debug;
	osThreadId_t detect;
	osThreadId_t info;
	osThreadId_t init;
	osThreadId_t monitor;
	osThreadId_t motor;
	osThreadId_t output;
	osThreadId_t pos_esti;
	osThreadId_t referee;
} Task_List_t;

#define TASK_CAN_FREQ_HZ (50)
#define TASK_CAN_INIT_DELAY (500)

#define TASK_CTRL_CHASSIS_FREQ_HZ (50)
#define TASK_CTRL_CHASSIS_INIT_DELAY (500)

#define TASK_DEBUG_FREQ_HZ (50)
#define TASK_DEBUG_INIT_DELAY (500)

/* Task_CAN */
extern osMessageQueueId_t motor_feedback_queue;
extern osMessageQueueId_t uwb_feedback_queue;
extern osMessageQueueId_t supercap_feedback_queue;

/* Task_CAN */
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


void Task_CAN(const void* argument);
void Task_CLI(const void* argument);
void Task_Comm(const void* argument);
void Task_CtrlChassis(const void* argument);
void Task_CtrlGimbal(const void* argument);
void Task_CtrlShoot(const void* argument);
void Task_Debug(const void* argument);
void Task_Detect(const void* argument);
void Task_Info(const void* argument);
void Task_Init(const void* argument);
void Task_Monitor(const void* argument);
void Task_Output(const void* argument);
void Task_PosEsti(const void* argument);
void Task_Referee(const void* argument);

/* Exported functions prototypes ---------------------------------------------*/
