/* 
	提供机器人运行信息的任务，主要包括OLED显示、LED显示等。

*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include Board相关的头文件 */
#include "bsp_led.h"

/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
#include "capacity.h"

/* Include Module相关的头文件。*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint32_t delay_ms = 1000u / TASK_INFO_FREQ_HZ;
static int result = 0;
static osStatus os_status = osOK;

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Info(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	float battery_voltage;
	float battery_percentage;
	float capacitot_percentage;
	
	/* 等待一段时间后再开始任务。*/
	osDelay(TASK_INFO_INIT_DELAY);
	BSP_LED_Set(BSP_LED_GRN, BSP_LED_ON, 1);

	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* 任务主体 */
		battery_percentage = Capacity_GetBatteryRemain(battery_voltage);
		
		BSP_LED_Set(BSP_LED_GRN, BSP_LED_TAGGLE, 1);
		osDelayUntil(&previous_wake_time, delay_ms);
	}
}
