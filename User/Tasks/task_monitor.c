/* 
	监控机器人运行情况相关的任务。

*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Board相关的头文件。*/
#include "io.h"

/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TASK_DEBUG_FREQ_HZ (50)
#define TASK_DEBUG_INIT_DELAY (500)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


void Task_Monitor(const void* argument) {
	uint32_t delay_tick = 1000U / TASK_DEBUG_FREQ_HZ;
	
	/* 处理硬件相关的初始化。*/
	
	/* 初始化完成后等待一段时间后再开始任务。*/
	osDelay(TASK_DEBUG_INIT_DELAY);
	while(1) {
		/* 任务主体。*/
		
		
		osDelayUntil(delay_tick);
	}
}
