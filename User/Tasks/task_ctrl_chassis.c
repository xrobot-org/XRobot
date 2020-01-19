/*
	底盘任务，用于控制底盘行为。
	
*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Board相关的头文件。*/
#include "io.h"

/* Include Device相关的头文件。*/
#include "can_device.h"

/* Include Component相关的头文件。*/
#include "mecanum.h"
#include "pid.h"

/* Include Module相关的头文件。*/
#include "chassis.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/


void Task_CtrlChassis(const void* argument) {
	uint32_t delay_tick = 1000U / TASK_CTRL_CHASSIS_FREQ_HZ;
	Task_List_t task_list = *(Task_List_t*)argument;
	
	
	/* 初始化完成后等待一段时间后再开始任务。*/
	osDelay(TASK_CTRL_CHASSIS_INIT_DELAY);
	
	/* 处理硬件相关的初始化。*/
	CAN_Device_t cd = {
		.chassis_alert = task_list.ctrl_chassis,
		.gimbal_alert = task_list.ctrl_gimbal,
		.uwb_alert = task_list.info,
		.supercap_alert = task_list.ctrl_chassis,
	};
	CAN_DeviceInit(&cd);
	
	Chassis_t chassis;
	Chassis_Init(&chassis);
	
	while(1) {
		/* 任务主体。*/
		

		osDelayUntil(delay_tick);
	}
}
