/*
  消息任务
  
  控制对外的指示装置，例如LED、OLED显示器等。
*/

/* Includes ----------------------------------------------------------------- */
#include "bsp\adc.h"
#include "bsp\led.h"
#include "bsp\usb.h"
#include "component\capacity.h"
#include "component\user_math.h"
#include "task\user_task.h"

/* Private typedef ---------------------------------------------------------- */
/* Private define ----------------------------------------------------------- */
/* Private macro ------------------------------------------------------------ */
/* Private variables -------------------------------------------------------- */
/* Private function --------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void Task_Info(void *argument) {
  const uint32_t delay_tick = osKernelGetTickFreq() / TASK_FREQ_HZ_INFO;
  Task_Param_t *task_param = (Task_Param_t *)argument;

  /* Task Setup */
  osDelay(TASK_INIT_DELAY_INFO);
  BSP_LED_Set(BSP_LED_GRN, BSP_LED_ON, 0.5f);

  uint32_t tick = osKernelGetTickCount();
  while (1) {
#ifdef DEBUG
    task_param->stack_water_mark.info = osThreadGetStackSpace(NULL);
#endif
    /* Task body */
    tick += delay_tick;

    float battery_volt = BSP_GetBatteryVolt();
    float battery_remain = Capacity_GetBatteryRemain(battery_volt);

    (void)battery_remain;

    BSP_LED_Set(BSP_LED_GRN, BSP_LED_TAGGLE, 1);

    osDelayUntil(tick);
  }
}
