/* 
	提供机器人运行信息的任务，主要包括OLED显示、LED显示等。

*/


/* Includes ------------------------------------------------------------------*/
#include "task\user_task.h"

#include "board\adc.h"
#include "board\led.h"
#include "board\usb.h"

#include "component\capacity.h"
#include "component\user_math.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Info(void *argument) {
	const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_MONITOR;
	
	/* Task Setup */
	osDelay(TASK_INIT_DELAY_INFO);
	BSP_LED_Set(BSP_LED_GRN, BSP_LED_ON, 0.5f);
	
	uint32_t tick = osKernelGetTickCount();
	while(1) {
		/* Task body */
		tick += delay_tick;
		
		float32_t battery_volt = BSP_GetBatteryVolt();
		float32_t battery_percentage = Capacity_GetBatteryRemain(battery_volt);
		
		BSP_LED_Set(BSP_LED_GRN, BSP_LED_TAGGLE, 1);
		
		osDelayUntil(tick);
	}
}
