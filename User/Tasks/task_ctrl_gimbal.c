#include "task_ctrl_gimbal.h"
#include "cmsis_os.h"

#include "io.h"
#include "oled.h"

#define GIMBAL_TASK_FREQ_HZ (100)
#define GIMBAL_TASK_STATUS_LED LED5

void GimbalTask(const void* argument) {
	LED_Set(GIMBAL_TASK_STATUS_LED, LED_ON);
	
	while(1) {

		LED_Set(GIMBAL_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(1000 / GIMBAL_TASK_FREQ_HZ);
	}
}
