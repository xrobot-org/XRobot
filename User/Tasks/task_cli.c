/* 
	运行命令行交互界面（Command Line Interface）。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Board相关的头文件 */
/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


void Task_CLI(const void *argument) {
	const uint32_t delay_ms = 1000u / TASK_CLI_FREQ_HZ;
	const Task_List_t task_list = *(Task_List_t*)argument;
	
	/* 等待一段时间后再开始任务。*/
	osDelay(TASK_CLI_INIT_DELAY);
	
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* 任务主体 */
		
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
