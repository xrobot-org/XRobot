/* 
	提供机器人运行信息的任务，主要包括OLED显示、LED显示等。

*/


/* Includes ------------------------------------------------------------------*/
#include "task_common.h"

/* Include 标准库 */
/* Include Board相关的头文件 */
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
static const uint32_t delay_ms = 1000u / TASK_INFO_FREQ_HZ;

/* Runtime status. */
int stat_in = 0;
osStatus os_stat_in = osOK;
#if INCLUDE_uxTaskGetStackHighWaterMark
uint32_t task_info_stack;
#endif

/* Private function prototypes -----------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
void Task_Info(void const *argument) {
	Task_Param_t *task_param = (Task_Param_t*)argument;
	
	float battery_voltage;
	float battery_percentage;
	float capacitot_percentage;
	
	/* Task Setup */
	osDelay(TASK_INFO_INIT_DELAY);
	BSP_LED_Set(BSP_LED_GRN, BSP_LED_ON, 0.5f);

	//BSP_USB_Printf("hello admin.");
	uint32_t previous_wake_time = osKernelSysTick();
	while(1) {
		/* Task body */
		battery_percentage = Capacity_GetBatteryRemain(battery_voltage);
		
		BSP_LED_Set(BSP_LED_GRN, BSP_LED_TAGGLE, 1);
		osDelayUntil(&previous_wake_time, delay_ms);
		
#if INCLUDE_uxTaskGetStackHighWaterMark
        task_info_stack = uxTaskGetStackHighWaterMark(NULL);
#endif
	}
}
