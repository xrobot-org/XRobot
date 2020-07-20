
/* Includes ------------------------------------------------------------------*/
#include "user_task.h"

#include "bsp\usb.h"

#include "device\referee.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static Referee_t ref;

/* Private function ----------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Referee(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_REFEREE;
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_REFEREE);
	
	Referee_Init(&ref, osThreadGetId());
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		Referee_StartReceiving(&ref);
		osThreadFlagsWait(REFEREE_SIGNAL_RAW_REDY, osFlagsWaitAll, osWaitForever);
		
		Referee_Parse(&ref);
		
		osDelayUntil(tick);
#ifdef DEBUG
		task_param->stack_water_mark.cli = uxTaskGetStackHighWaterMark(NULL);
#endif
	}
}
