
/* Includes ------------------------------------------------------------------*/
#include "task\user_task.h"

#include "board\usb.h"

#include "device\referee.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static Referee_t ref;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Referee(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_REFEREE;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_REFEREE);
	
	Referee_Init(&ref, osThreadGetId());
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		Referee_StartReceivingHeader(&ref);
		osThreadFlagsWait(REFEREE_SIGNAL_RAW_REDY, osFlagsWaitAll, osWaitForever);
		
		if (Referee_CheckHeader(&ref)) {
			Referee_StartReceivingCMDID(&ref);
			osThreadFlagsWait(REFEREE_SIGNAL_RAW_REDY, osFlagsWaitAll, osWaitForever);
			
			Referee_StartReceivingData(&ref);
			osThreadFlagsWait(REFEREE_SIGNAL_RAW_REDY, osFlagsWaitAll, osWaitForever);
			
			Referee_StartReceivingTail(&ref);
			osThreadFlagsWait(REFEREE_SIGNAL_RAW_REDY, osFlagsWaitAll, osWaitForever);
		}
		osDelayUntil(tick);
	}
}
