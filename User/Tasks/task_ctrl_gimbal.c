/*
	控制云台。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Board相关的头文件。*/
#include "io.h"

/* Include Device相关的头文件。*/
#include "can_device.h"

/* Include Component相关的头文件。*/
#include "pid.h"

/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/


void Task_CtrlGimbal(const void* argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_CAN_FREQ_HZ;
	Task_List_t task_list = *(Task_List_t*)argument;
	
	/* 初始化完成后等待一段时间后再开始任务。*/
	osDelay(TASK_CAN_INIT_DELAY);

	while(1) {
		/* 任务主体。*/
		osThreadFlagsWait(CAN_DEVICE_SIGNAL_MOTOR_RECV, osFlagsWaitAny, 0);
		
		osDelayUntil(delay_tick);
	}
}
