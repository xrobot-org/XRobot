/*
	临时任务用于调试，也用作任务模板。
	
	所有已经写完的程序都要按照本文件添加注释。
	不同组的文件有细微差别，参考本组内的文件编写。
	
*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库*/
/* Include Board相关的头文件 */
/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = 1000u / TASK_DEBUG_FREQ_HZ;
static int result = 0;
static osStatus os_status = osOK;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Debug(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_DEBUG_INIT_DELAY);
	
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task */
		
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
