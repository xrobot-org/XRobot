/*
	控制云台。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Board相关的头文件 */
/* Include Device相关的头文件 */
#include "can_device.h"

/* Include Component相关的头文件 */
#include "mixer.h"
#include "pid.h"

/* Include Module相关的头文件 */
#include "chassis.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
CAN_Device_t cd;

/* Private function prototypes -----------------------------------------------*/


void Task_CtrlGimbal(const void *argument) {
	const uint32_t delay_ms = 1000u / TASK_CAN_FREQ_HZ;
	const Task_List_t task_list = *(Task_List_t*)argument;
	
	/* 等待一段时间后再开始任务。*/
	osDelay(TASK_CAN_INIT_DELAY);
	
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* 任务主体 */
		osSignalWait(CAN_DEVICE_SIGNAL_GIMBAL_RECV, osWaitForever);
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
