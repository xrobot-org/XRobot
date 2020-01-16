/* 
	提供机器人运行信息的任务，主要包括OLED显示、LED显示等。

*/

/* Includes ------------------------------------------------------------------*/
/* Include 自身的头文件，OS头文件。*/
#include "task_debug.h"
#include "cmsis_os2.h"

/* Include Board相关的头文件。*/
#include "board.h"

/* Include Device相关的头文件。*/
/* Include Component相关的头文件。*/
#include "capacity.h"

/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TASK_INFO_FREQ_HZ (5)
#define TASK_INFO_INIT_DELAY (500)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


void Task_Info(const void* argument) {
	uint32_t delay_tick = 1000U / TASK_INFO_FREQ_HZ;
	float battery_voltage;
	float battery_percentage;
	float capacitot_percentage;
	
	/* 处理硬件相关的初始化。*/
	
	/* 初始化完成后等待一段时间后再开始任务。*/
	osDelay(TASK_INFO_INIT_DELAY);
	while(1) {
		/* 任务主体。*/
		//battery_voltage = Board_GetBatteryVoltage();
		battery_percentage = Capacity_GetBatteryRemain(battery_voltage);
		
		//OLED
		
		osDelayUntil(delay_tick);
	}
}
