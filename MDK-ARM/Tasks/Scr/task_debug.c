#include "task_debug.h"
#include "main.h"
#include "cmsis_os.h"

#include "bsp_io.h"

void DebugTask(void const * argument) {
	uint32_t last_tick = osKernelSysTick();
	
	LED_Set(LED8, LED_ON);
	
	while(1) {

		osDelayUntil(&last_tick, 1);
	}
}
