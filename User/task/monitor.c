/*
  监控任务。
  
  监控系统运行情况，记录错误。
*/

/* Includes ----------------------------------------------------------------- */
#include "bsp\adc.h"
#include "bsp\led.h"
#include "bsp\buzzer.h"
#include "bsp\usb.h"
#include "component/capacity.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
float battery_remain;

/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Task_Monitor(void *argument) {
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_MONITOR;
  Task_Param_t *task_param = (Task_Param_t *)argument;

  /* Task Setup */
  osDelay(TASK_INIT_DELAY_REFEREE);

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_param->stack_water_mark.monitor = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;
    float battery_volt = BSP_GetBatteryVolt();
    battery_remain = Capacity_GetBatteryRemain(battery_volt);
    
    if (battery_remain < 0.2f) {
      BSP_LED_Set(BSP_LED_RED, BSP_LED_TAGGLE, 1);
    } else {
      BSP_LED_Set(BSP_LED_RED, BSP_LED_OFF, 1);
    }

    osDelayUntil(tick);
  }
}
