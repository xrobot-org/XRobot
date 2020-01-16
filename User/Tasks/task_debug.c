/*
	临时任务用于调试，也用作任务模板。
	
	所有任务都要define一个“任务运行频率”和“初始化延时”。
	
	现阶段所有任务不使用LED等，只使用完成所需要的必须品。
	已有的LED控制等删除掉。
	
	所有已经写完的程序都要按照本文件添加注释。
	不同组的文件有细微差别，参考本组内的文件编写。
	
*/


/* Includes ------------------------------------------------------------------*/
/* Include 自身的头文件，OS头文件。*/
#include "task_debug.h"
#include "cmsis_os2.h"

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


void Task_Debug(const void* argument) {
	uint32_t delay_tick = 1000U / TASK_DEBUG_FREQ_HZ;
	
	/* 处理硬件相关的初始化。*/
	
	/* 初始化完成后等待一段时间后再开始任务。*/
	osDelay(TASK_DEBUG_INIT_DELAY);
	while(1) {
		/* 任务主体。*/
		
		
		osDelayUntil(delay_tick);
	}
}
