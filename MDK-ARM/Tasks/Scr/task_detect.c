#include "task_detect.h"
#include "main.h"
#include "cmsis_os.h"

#include "bsp_io.h"

#define DETECT_TASK_FREQ_HZ (100)
#define DETECT_TASK_STATUS_LED LED3

void DetectTask(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	
	LED_Set(DETECT_TASK_STATUS_LED, LED_ON);
	
	while(1) {

		LED_Set(DETECT_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(&last_tick, (1000 / DETECT_TASK_FREQ_HZ));
	}
}
