/*
	底盘任务，用于控制底盘。
	
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
static const uint32_t delay_ms = 1000U / TASK_CTRL_CHASSIS_FREQ_HZ;
static int result = 0;
static osStatus os_status = osOK;

static CAN_Device_t cd;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlChassis(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	
	/* 等待一段时间后再开始任务。*/
	osDelay(TASK_CTRL_CHASSIS_INIT_DELAY);
	
	/* 初始化硬件 */
	cd.chassis_alert = task_param->thread.ctrl_chassis;
	cd.gimbal_alert = task_param->thread.ctrl_gimbal;
	cd.uwb_alert = task_param->thread.info;
	cd.supercap_alert = task_param->thread.ctrl_chassis;
	
	CAN_DeviceInit(&cd);
	
	Chassis_t chassis;
	Chassis_Init(&chassis);
	
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* 任务主体 */
		

		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
