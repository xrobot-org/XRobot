/* 
	运行命令行交互界面（Command Line Interface）。

*/

/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
#include "bsp_usb.h"

/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = 1000u / TASK_CLI_FREQ_HZ;
static int result = 0;
static osStatus os_status = osOK;

static const char* const welcom_message = 
	" -----------------------------------------------------\r\n"
	" RoboMaster 2020. Qingdao University.\r\n"
	" -----------------------------------------------------\r\n"
	" Robot Model: "
#if defined ROBOT_TYPE_INFANTRY	
	"Infantry\r\n"
#elif defined ROBOT_TYPE_HERO
	"Hero\r\n"
#elif defined ROBOT_TYPE_ENGINEER
	"Engineer\r\n"
#elif defined ROBOT_TYPE_DRONE
	"Drone\r\n"
#elif defined ROBOT_TYPE_SENTRY
	"Sentry\r\n"
#endif
	" Firmware Version: 0.0.1\r\n"
	" -----------------------------------------------------\r\n"
	" FreeRTOS CLI. Type 'help' to view a list of registered commands.\r\n"
	"\r\n";

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_CLI(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	/* Task Setup */
	osDelay(TASK_CLI_INIT_DELAY);
	
	//FreeRTOS_CLIRegisterCommand( &xDelCommand );
	BSP_USB_Printf("Please press Enter to activate this console.\r\n");
	//TODO: Wait for enter;
	osSignalWait(0, 1000);
	
	BSP_USB_Printf(welcom_message);
	
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task */
		BSP_USB_Printf("rm>");
		osSignalWait(0, osWaitForever);
		BSP_USB_Printf("\r\n");
			
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
