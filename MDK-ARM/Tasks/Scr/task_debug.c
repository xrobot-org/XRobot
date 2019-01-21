#include "task_debug.h"
#include "main.h"
#include "cmsis_os.h"

#include "bsp_io.h"

#define DEBUG_TASK_FREQ_HZ (10)
#define DEBUG_TASK_STATUS_LED LED8

void DebugTask(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	
	LED_Set(DEBUG_TASK_STATUS_LED, LED_ON);
	
	while(1) {

		LED_Set(DEBUG_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(&last_tick, 1);
	}
}
