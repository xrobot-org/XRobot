/*
	处理CAN总线相关的数据发送和接收。

*/

/* Includes ------------------------------------------------------------------*/
/* Include 自身的头文件*/
#include "task_can.h"

/* Include Board相关的头文件。*/
#include "io.h"

/* Include Device相关的头文件。*/
#include "can_device.h"

/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define TASK_CAN_FREQ_HZ (50)
#define TASK_CAN_INIT_DELAY (500)

void Task_CAN(const void* argument) {
	uint32_t delay_tick = osKernelGetTickFreq() / TASK_CAN_FREQ_HZ;
	uint32_t receive_flag = 0u;
	
	motor_feedback_queue = osMessageQueueNew(5, 5 * sizeof(CAN_Motor_Feedback_t), NULL);
	uwb_feedback_queue = osMessageQueueNew(1, 1 * sizeof(CAN_UWB_Feedback_t), NULL);
	supercap_feedback_queue = osMessageQueueNew(1, 1 * sizeof(CAN_SuperCap_Feedback_t), NULL);
	
	/* 处理硬件相关的初始化。*/
	CAN_Device_Init(osThreadGetId());
	
	/* 初始化完成后等待一段时间后再开始任务。*/
	osDelay(TASK_CAN_INIT_DELAY);
	while(1) {
		switch (osThreadFlagsWait(0x0001U, osFlagsWaitAny, osWaitForever)) {
			case CAN_DEVICE_SIGNAL_MOTOR_RECV:
				
				break;
			case CAN_DEVICE_SIGNAL_UWB_RECV:
				break;
			case CAN_DEVICE_SIGNAL_SUPERCAP_RECV:
				break;
		}
		
	}
}
