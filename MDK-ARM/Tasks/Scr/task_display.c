#include "task_display.h"
#include "main.h"
#include "cmsis_os.h"

#include "bsp_oled.h"
#include "bsp_io.h"

#define DISPLAY_TASK_FREQ_HZ (10)

void DisplayTask(void const * argument) {
	uint32_t last_tick = osKernelSysTick();
	
	LED_Set(LED1, LED_ON);
	
	while (1) {
		OLED_Refresh();
		LED_Set(LED1, LED_TAGGLE);
		
		osDelayUntil(&last_tick, (1000 / DISPLAY_TASK_FREQ_HZ));
	}
}
