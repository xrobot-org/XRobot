#include "task_gimbal.h"
#include "main.h"
#include "cmsis_os.h"

#include "bsp_io.h"
#include "bsp_oled.h"

#define GIMBAL_TASK_FREQ_HZ (100)
#define GIMBAL_TASK_STATUS_LED LED5

void GimbalTask(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	
	LED_Set(GIMBAL_TASK_STATUS_LED, LED_ON);
	
	while(1) {

		LED_Set(GIMBAL_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(&last_tick, (1000 / GIMBAL_TASK_FREQ_HZ));
	}
}
