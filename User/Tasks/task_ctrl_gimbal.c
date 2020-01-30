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
#include "ahrs.h"

/* Include Module相关的头文件 */
#include "gimbal.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = 1000u / TASK_CTRL_GIMBAL_FREQ_HZ;
static int result = 0;
static osStatus os_status = osOK;

static CAN_Device_t cd;


/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CtrlGimbal(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* 等待一段时间后再开始任务。*/
	osSignalWait(TASK_SIGNAL_POSESTI_READY, osWaitForever);
	
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* 任务主体 */
		osSignalWait(CAN_DEVICE_SIGNAL_GIMBAL_RECV, osWaitForever);
		
		AHRS_t  *gimbal_ahrs;
		
		osEvent evt = osMessageGet(task_param->message.ahrs, osWaitForever);
		if (evt.status == osEventMessage)
			gimbal_ahrs = evt.value.p; 
		
		osPoolFree(task_param->pool.ahrs, gimbal_ahrs);
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
