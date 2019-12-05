#include "task_chassis_ctrl.h"
#include "main.h"
#include "cmsis_os.h"

#include "io.h"

#define CHASSIS_TASK_FREQ_HZ (50)
#define CHASSIS_TASK_STATUS_LED LED1

void ChassisTask(const void* argument) {
	uint32_t last_tick = osKernelSysTick();
	
	LED_Set(CHASSIS_TASK_STATUS_LED, LED_ON);
	
	while(1) {

		LED_Set(CHASSIS_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(1000 / CHASSIS_TASK_FREQ_HZ);
	}
}
