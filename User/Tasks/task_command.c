/* 
	接收解释指令。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库*/
/* Include Board相关的头文件 */
/* Include Device相关的头文件。*/
#include "dr16.h"

/* Include Component相关的头文件。*/
/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = 1000u / TASK_COMMAND_FREQ_HZ;
static int result = 0;
static osStatus os_status = osOK;

DR16_t dr16;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Comm(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_COMMAND_INIT_DELAY);
	
	dr16.received_alert = task_param->thread.comm;
	DR16_Init(&dr16);
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task */
		result += DR16_StartReceiving(&dr16);
		osSignalWait(DR16_SIGNAL_RAW_REDY, osWaitForever);
		result += DR16_Parse(&dr16);
		
		osSignalSet(task_param->thread.ctrl_chassis, DR16_SIGNAL_DATA_REDY);
		osSignalSet(task_param->thread.ctrl_gimbal, DR16_SIGNAL_DATA_REDY);
		osSignalSet(task_param->thread.ctrl_shoot, DR16_SIGNAL_DATA_REDY);
		
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
