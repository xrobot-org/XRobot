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

/* Runtime status. */
int stat_mo = 0;
osStatus_t os_stat_mo = osOK;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Monitor(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_MONITOR;
	const Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_MONITOR);
	
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		
		osDelayUntil(tick);
	}
}
