#include "task_shoot_ctrl.h"
#include "main.h"
#include "cmsis_os.h"

#include "io.h"
#include "oled.h"

#define SHOOT_TASK_FREQ_HZ (100)
#define SHOOT_TASK_STATUS_LED LED8

void ShootTask(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	
	LED_Set(SHOOT_TASK_STATUS_LED, LED_ON);
	
	while(1) {
		
		LED_Set(SHOOT_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(1000 / SHOOT_TASK_FREQ_HZ);
	}
}

