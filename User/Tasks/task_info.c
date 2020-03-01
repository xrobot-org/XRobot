/* 
	提供机器人运行信息的任务，主要包括OLED显示、LED显示等。

*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
#include "bsp_adc.h"
#include "bsp_led.h"
#include "bsp_usb.h"

/* Include Device相关的头文件 */
/* Include Component相关的头文件 */
#include "capacity.h"

/* Include Module相关的头文件 */
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Info(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_INFO;
	const Task_Param_t *task_param = (Task_Param_t*)argument;
	
	float capacitor_percentage;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_INFO);
	BSP_LED_Set(BSP_LED_GRN, BSP_LED_ON, 0.5f);
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		float battery_voltage = BSP_GetBatteryVoltage();
		float battery_percentage = Capacity_GetBatteryRemain(battery_voltage);
		
		BSP_LED_Set(BSP_LED_GRN, BSP_LED_TAGGLE, 1);
		
		osDelayUntil(tick);
	}
}
