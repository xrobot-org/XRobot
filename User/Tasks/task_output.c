/*
	输出任务，用于各种动作器的控制。
	接收来自其他任务的信号或控制量，进行反馈控制
	
*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"
#include "cmsis_os.h"

/* Include Board相关的头文件。*/
#include "io.h"

/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TASK_OUTPUT_FREQ_HZ (50)
#define TASK_OUTPUT_INIT_DELAY (500)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


void Task_Output(const void* argument) {
	/* 处理硬件相关的初始化。*/
	uint32_t delay_tick = 1000U / TASK_OUTPUT_FREQ_HZ;
	
	/* 初始化完成后等待一段时间后再开始任务。*/
	osDelay(TASK_OUTPUT_INIT_DELAY);
	while(1) {
		/* 任务主体。*/
		
		
		osDelayUntil(delay_tick);
	}
}
