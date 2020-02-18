
/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = 1000u / TASK_REFEREE_FREQ_HZ;

/* Runtime status. */
int stat_re = 0;
osStatus os_stat_re = osOK;
#if INCLUDE_uxTaskGetStackHighWaterMark
uint32_t task_referee_stack;
#endif

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Referee(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_REFEREE_INIT_DELAY);
	
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task body */
		
		
		osDelayUntil(&previous_wake_time, delay_ms);
		
#if INCLUDE_uxTaskGetStackHighWaterMark
        task_referee_stack = uxTaskGetStackHighWaterMark(NULL);
#endif
	}
}
