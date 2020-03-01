/* 
	接收解释指令。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
#include <string.h>

/* Include Board相关的头文件 */
#include "bsp_usb.h"

/* Include Device相关的头文件 */
#include "dr16.h"

/* Include Component相关的头文件 */
/* Include Module相关的头文件 */
#include "chassis.h"
#include "gimbal.h"
#include "shoot.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static DR16_t dr16;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Command(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_COMMAND);
	
	dr16.received_alert = osThreadGetId();
	DR16_Init(&dr16);
	
	while(1) {
		/* Task body */
		DR16_StartReceiving(&dr16);
		osSignalWait(DR16_SIGNAL_RAW_REDY, osWaitForever);
		DR16_Parse(&dr16);
		
		osSignalSet(task_param->thread.ctrl_chassis, DR16_SIGNAL_DATA_REDY);
		osSignalSet(task_param->thread.ctrl_gimbal, DR16_SIGNAL_DATA_REDY);
		osSignalSet(task_param->thread.ctrl_shoot, DR16_SIGNAL_DATA_REDY);
	}
}
