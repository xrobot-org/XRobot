#include "task_display.h"
#include "main.h"
#include "cmsis_os.h"

#include "oled.h"
#include "io.h"

#define DISPLAY_TASK_FREQ_HZ (100)
#define DISPLAY_TASK_STATUS_LED LED4

void DisplayTask(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	
	LED_Set(DISPLAY_TASK_STATUS_LED, LED_ON);
	
	while (1) {
		OLED_Refresh();
		
		LED_Set(DISPLAY_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(1000 / DISPLAY_TASK_FREQ_HZ);
	}
}
