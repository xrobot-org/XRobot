/* 
	监控机器人运行情况相关的任务。

*/


/* Includes ------------------------------------------------------------------*/
#include "user_task.h"

#include "bsp\usb.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Monitor(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_MONITOR;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_REFEREE);
	
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		
		osDelayUntil(tick);
	}
}
