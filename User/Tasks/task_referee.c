#include "task_comm.h"
#include "main.h"
#include "cmsis_os.h"

#include "io.h"

#include "protocol.h"

#define COMM_TASK_FREQ_HZ (20)
#define COMM_TASK_STATUS_LED LED2

void CommTask(const void* argument) {
	LED_Set(COMM_TASK_STATUS_LED, LED_ON);
	
	while(1) {
		
		LED_Set(COMM_TASK_STATUS_LED, LED_TAGGLE);
		osDelayUntil(1000 / COMM_TASK_FREQ_HZ);
	}
}
