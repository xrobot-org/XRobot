/* 
	接收解释指令。

*/

/* Includes ------------------------------------------------------------------*/
#include "user_task.h"

#include <string.h>

#include "bsp\usb.h"

#include "component\config.h"

#include "device\dr16.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static DR16_t dr16;
static CMD_RC_t rc;
static CMD_t cmd;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Command(void *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	task_param->messageq.cmd = osMessageQueueNew(3u, sizeof(CMD_t), NULL);
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_COMMAND);
	
	DR16_Init(&dr16, osThreadGetId());
	CMD_Init(&cmd, &(Config_GetUser(CONFIG_USER_DEFAULT)->param.cmd));
	
	while(1) {
		/* Task body */
		DR16_StartReceiving(&dr16);
		osThreadFlagsWait(DR16_SIGNAL_RAW_REDY, osFlagsWaitAll, osWaitForever);
		
		if (DR16_ParseRC(&dr16, &rc)) {
			DR16_Restart();
			
		} else {
			CMD_Parse(&rc, &cmd);
			osStatus_t os_status = osMessageQueuePut(task_param->messageq.cmd, &cmd, 0, 0);
			
			if (os_status != osOK) {
			}
		}
	}
}
