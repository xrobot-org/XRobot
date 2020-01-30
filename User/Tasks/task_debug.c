/*
	临时任务用于调试，也用作任务模板。
	
	所有已经写完的程序都要按照本文件添加注释。
	不同组的文件有细微差别，参考本组内的文件编写。
	
*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


void Task_Debug(const void *argument) {
	const uint32_t delay_ms = 1000u / TASK_DEBUG_FREQ_HZ;
	const Task_List_t task_list = *(Task_List_t*)argument;
	
	/* 等待一段时间后再开始任务。*/
	osDelay(TASK_DEBUG_INIT_DELAY);
	uint32_t previous_wake_time = osKernelSysTick();
	
	
	while(1) {
		/* 任务主体 */
		
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
