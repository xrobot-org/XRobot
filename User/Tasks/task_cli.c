/* 
	运行命令行交互界面（Command Line Interface）。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Board相关的头文件。*/
/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/


void Task_CLI(const void* argument) {
	uint32_t delay_tick = osKernelGetTickFreq() / TASK_CAN_FREQ_HZ;
	Task_List_t task_list = *(Task_List_t*)argument;
	
	/* 初始化完成后等待一段时间后再开始任务。*/
	osDelay(TASK_CAN_INIT_DELAY);
	
	/* 处理硬件相关的初始化。*/
	
	while(1) {
		/* 任务主体。*/
		
		
		osDelayUntil(delay_tick);
	}
}
