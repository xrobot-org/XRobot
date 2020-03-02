/* 
	监控机器人运行情况相关的任务。

*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
#include "bsp_usb.h"

/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = osKernelSysTickFrequency / TASK_FREQ_HZ_MONITOR;

/* Runtime status. */
int stat_mo = 0;
osStatus os_stat_mo = osOK;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Monitor(void const *argument) {
	//Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_MONITOR);
	
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task body */
		
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
